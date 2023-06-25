// Copyright 2008-2023 Aerospike, Inc.
//
// Portions may be licensed to Aerospike, Inc. under one or more contributor
// license agreements.
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not
// use this file except in compliance with the License. You may obtain a copy of
// the License at http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations under
// the License.

#include <grpcpp/grpcpp.h>
#include <thread>
#include <jwt-cpp/jwt.h>
#include <aerospike/as_log_macros.h>

#include "aerospike_auth_proxy.h"
#include "auth.grpc.pb.h"

static ProxyAccessToken* parseToken(std::string token) {
	auto decoded = jwt::decode(token);
	cf_clock issuedAt =
		std::chrono::time_point_cast<std::chrono::milliseconds>(decoded.get_issued_at()).time_since_epoch().count();
	cf_clock expiresAt =
		std::chrono::time_point_cast<std::chrono::milliseconds>(decoded.get_expires_at()).time_since_epoch().count();
	int32_t ttl = expiresAt - issuedAt;
	if (ttl <= 0) {
		throw ("token 'iat' > 'exp'");
	}

	long expiry = cf_getms() + ttl;
	ProxyAccessToken* accessToken = new ProxyAccessToken(expiry, ttl, token);
	return accessToken;
}

ProxyAuthTokenManager::ProxyAuthTokenManager(as_config* config, std::shared_ptr<Channel> channel) {
	this->config = config;
	this->channel = channel;
	tokenRef = new ProxyAccessToken(cf_getms(), 0, "");
	consecutiveRefreshErrors = 0;
	terminated = false;
	if (config->user) {
		std::thread t(&ProxyAuthTokenManager::refreshLoop, this);
		t.detach();
	}
}

void ProxyAuthTokenManager::setCredentials(grpc::ClientContext* context) {
	if (config->user) {
		context->set_credentials(grpc::MetadataCredentialsFromPlugin(
			std::unique_ptr<grpc::MetadataCredentialsPlugin>(
				new ProxyAuthenticator(tokenRef.load()->token)
			)));
	}
}

void ProxyAuthTokenManager::shutdown() {
	terminated = true;
}

void ProxyAuthTokenManager::refreshLoop() {
	while (!this->terminated.load()) {
		if (cf_getms() >= nextRefreshTime()) {
			// Time to refresh the token.
			std::unique_ptr<AuthService::Stub> stub = AuthService::NewStub(channel);
			AerospikeAuthRequest request;
			AerospikeAuthResponse response;

			request.set_username(config->user);
			request.set_password(config->password);

			ClientContext context;

			Status status = stub->Get(&context, request, &response);
			if (status.ok()) {
				try {
					ProxyAccessToken* newToken = parseToken(response.token());
					ProxyAccessToken* oldToken = tokenRef.load();
					tokenRef.store(newToken);
					delete oldToken;
					consecutiveRefreshErrors.store(0);
				} catch (std::string err) {
					consecutiveRefreshErrors++;
					as_log_error("Error parsing access token - %s", err.c_str());
				}
			} else {
				consecutiveRefreshErrors++;
				as_log_error("Error fetching access token code:%d message:'%s' details:'%s')",
							 status.error_code(),
							 status.error_message().c_str(),
							 status.error_details().c_str())
			}
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
}

cf_clock ProxyAuthTokenManager::nextRefreshTime() {
	ProxyAccessToken* token = tokenRef.load();
	long ttl = token->ttl;
	long delay = (long)std::floor(ttl*refreshAfterFraction);

	if (ttl - delay < refreshMinTime) {
		// We need at least refreshMinTimeMillis to refresh, schedule
		// immediately.
		delay = ttl - refreshMinTime;
	}

	if (token->hasExpired()) {
		// Force immediate refresh.
		delay = 0;
	}

	if (delay==0 && consecutiveRefreshErrors.load() > 0) {
		// If we continue to fail then schedule will be too aggressive on fetching new token. Avoid that by increasing
		// fetch delay.

		delay = (long)(std::pow(2, consecutiveRefreshErrors.load())*1000);
		if (delay > maxExponentialBackOff) {
			delay = maxExponentialBackOff;
		}

		// Handle wrap around.
		if (delay < 0) {
			delay = 0;
		}
	}

	return cf_getms() + delay;
}
