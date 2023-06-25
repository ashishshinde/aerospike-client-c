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

#include <iostream>
#include <thread>

#include "aerospike/aerospike.h"
#include "aerospike/as_config.h"
#include "aerospike_auth_proxy.h"

int main() {
	as_config config;
	as_config_init(&config);
	strcpy(config.user, "admin");
	strcpy(config.password, "admin");
	printf("Here\n");

	std::string host = "localhost";
	int port = 4000;
	if (!as_config_add_hosts(&config, host.c_str(), port)) {
		printf("Invalid host(s) %s port %d\n", host.c_str(), port);
		return -1;
	}

	std::string address = format("%s:%d", host.c_str(), port);
	ProxyAuthTokenManager tokenManager = ProxyAuthTokenManager(&config, grpc::CreateChannel(
		address,
		grpc::InsecureChannelCredentials()
	));

	std::this_thread::sleep_for(std::chrono::milliseconds(10000));
	ClientContext context;
	tokenManager.setCredentials(&context);
	printf("%s", context.credentials()->DebugString().c_str());
}
