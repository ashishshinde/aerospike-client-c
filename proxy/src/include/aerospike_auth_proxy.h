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

#pragma once

#include <grpcpp/grpcpp.h>
#include <citrusleaf/cf_clock.h>

#include "aerospike_proxy_cluster.h"

class ProxyAuthenticator : public grpc::MetadataCredentialsPlugin {
	public:
	ProxyAuthenticator(const grpc::string &token) : token_(token) {}

	grpc::Status GetMetadata(
		grpc::string_ref service_url, grpc::string_ref method_name,
		const grpc::AuthContext &channel_auth_context,
		std::multimap<grpc::string, grpc::string>* metadata) override {
		metadata->insert(std::make_pair("Authorization", token_));
		return grpc::Status::OK;
	}

	private:
	grpc::string token_;
};

class ProxyAccessToken {
	public:
	/**
	 * Local token expiry timestamp in millis.
	 */
	long expiry;
	/**
	 * Remaining time to live for the token in millis.
	 */
	long ttl;
	/**
	 * An access token for Aerospike proxy.
	 */
	std::string token;

	ProxyAccessToken(long expiry, long ttl, std::string token) {
		this->expiry = expiry;
		this->ttl = ttl;
		this->token = std::move(token);
	}

	bool hasExpired() {
		return cf_getms() > expiry;
	}
};

class ProxyAuthTokenManager {
	private:
	/**
	 * A conservative estimate of minimum amount of time in millis it takes for
	 * token refresh to complete. Auto refresh should be scheduled at least
	 * this amount before expiry, i.e, if remaining expiry time is less than
	 * this amount refresh should be scheduled immediately.
	 */
	const static int refreshMinTime = 5000;

	/**
	 * A cap on refresh time in millis to throttle an auto refresh requests in
	 * case of token refresh failure.
	 */
	const static int maxExponentialBackOff = 15000;

	/**
	 * Fraction of token expiry time to elapse before scheduling an auto
	 * refresh.
	 *
	 * @see AuthTokenManager#refreshMinTime
	 */
	constexpr static float refreshAfterFraction = 0.95f;

	std::atomic<int> consecutiveRefreshErrors;
	std::atomic<ProxyAccessToken*> tokenRef;
	as_config* config;
	std::atomic<bool> terminated;
	std::shared_ptr<Channel> channel;

	cf_clock nextRefreshTime();
	void refreshLoop();

	public:
	ProxyAuthTokenManager(as_config* config, std::shared_ptr<Channel> channel);
	void setCredentials(ClientContext* context);
	void shutdown();
};