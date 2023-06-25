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

#include "aerospike_proxy_cluster.h"

AerospikeProxyCluster::AerospikeProxyCluster(as_config* config, as_error* err) {
	this->config = config;
	maxChannels = std::max((uint32_t)8, std::max(config->async_max_conns_per_node, config->max_conns_per_node));
	maxCtrlChannels = 1;
	channels = new std::shared_ptr<Channel>[maxChannels];
	ctrlChannels = new std::shared_ptr<Channel>[1];

	if (config->hosts->size==0) {
		as_error_update(err, AEROSPIKE_ERR_CLIENT, "Empty hosts")
		return;
	}

	// Use first host as address for now.
	as_host* seed = (as_host*)as_vector_get(config->hosts, 0);

	// TODO: handle ipv6 addresses
	std::string address = format("%s:%d", seed->name, seed->port);

	for (int i = 0; i < maxChannels; i++) {
		channels[i] = grpc::CreateChannel(
			address,
			grpc::InsecureChannelCredentials()
		);
	}

	for (int i = 0; i < maxCtrlChannels; i++) {
		ctrlChannels[i] = grpc::CreateChannel(
			address,
			grpc::InsecureChannelCredentials()
		);
	}
}