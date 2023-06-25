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

#include "aerospike_strings.h"
#include "kvs.grpc.pb.h"
#include <aerospike/as_vector.h>
#include <aerospike/as_config.h>

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

class AerospikeProxyCluster {
	private:
	as_config* config;
	int maxChannels;
	int maxCtrlChannels;
	std::shared_ptr<Channel>* channels;
	std::shared_ptr<Channel>* ctrlChannels;

	public:
	AerospikeProxyCluster(as_config* config, as_error* err);

};