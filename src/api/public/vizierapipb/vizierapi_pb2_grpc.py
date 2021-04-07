# Copyright 2018- The Pixie Authors.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# Generated by the gRPC Python protocol compiler plugin. DO NOT EDIT!
"""Client and server classes corresponding to protobuf-defined services."""
import grpc

from src.api.public.vizierapipb import vizierapi_pb2 as src_dot_api_dot_public_dot_vizierapipb_dot_vizierapi__pb2


class VizierServiceStub(object):
    """The API that manages all communication with a particular Vizier cluster.
    """

    def __init__(self, channel):
        """Constructor.

        Args:
            channel: A grpc.Channel.
        """
        self.ExecuteScript = channel.unary_stream(
                '/pl.api.vizierpb.VizierService/ExecuteScript',
                request_serializer=src_dot_api_dot_public_dot_vizierapipb_dot_vizierapi__pb2.ExecuteScriptRequest.SerializeToString,
                response_deserializer=src_dot_api_dot_public_dot_vizierapipb_dot_vizierapi__pb2.ExecuteScriptResponse.FromString,
                )
        self.HealthCheck = channel.unary_stream(
                '/pl.api.vizierpb.VizierService/HealthCheck',
                request_serializer=src_dot_api_dot_public_dot_vizierapipb_dot_vizierapi__pb2.HealthCheckRequest.SerializeToString,
                response_deserializer=src_dot_api_dot_public_dot_vizierapipb_dot_vizierapi__pb2.HealthCheckResponse.FromString,
                )


class VizierServiceServicer(object):
    """The API that manages all communication with a particular Vizier cluster.
    """

    def ExecuteScript(self, request, context):
        """Execute a script on the Vizier cluster and stream the results of that execution.
        """
        context.set_code(grpc.StatusCode.UNIMPLEMENTED)
        context.set_details('Method not implemented!')
        raise NotImplementedError('Method not implemented!')

    def HealthCheck(self, request, context):
        """Start a stream to receive health updates from the Vizier service. For most practical 
        purposes, users should only need `ExecuteScript()` and can safely ignore this call.
        """
        context.set_code(grpc.StatusCode.UNIMPLEMENTED)
        context.set_details('Method not implemented!')
        raise NotImplementedError('Method not implemented!')


def add_VizierServiceServicer_to_server(servicer, server):
    rpc_method_handlers = {
            'ExecuteScript': grpc.unary_stream_rpc_method_handler(
                    servicer.ExecuteScript,
                    request_deserializer=src_dot_api_dot_public_dot_vizierapipb_dot_vizierapi__pb2.ExecuteScriptRequest.FromString,
                    response_serializer=src_dot_api_dot_public_dot_vizierapipb_dot_vizierapi__pb2.ExecuteScriptResponse.SerializeToString,
            ),
            'HealthCheck': grpc.unary_stream_rpc_method_handler(
                    servicer.HealthCheck,
                    request_deserializer=src_dot_api_dot_public_dot_vizierapipb_dot_vizierapi__pb2.HealthCheckRequest.FromString,
                    response_serializer=src_dot_api_dot_public_dot_vizierapipb_dot_vizierapi__pb2.HealthCheckResponse.SerializeToString,
            ),
    }
    generic_handler = grpc.method_handlers_generic_handler(
            'pl.api.vizierpb.VizierService', rpc_method_handlers)
    server.add_generic_rpc_handlers((generic_handler,))


 # This class is part of an EXPERIMENTAL API.
class VizierService(object):
    """The API that manages all communication with a particular Vizier cluster.
    """

    @staticmethod
    def ExecuteScript(request,
            target,
            options=(),
            channel_credentials=None,
            call_credentials=None,
            insecure=False,
            compression=None,
            wait_for_ready=None,
            timeout=None,
            metadata=None):
        return grpc.experimental.unary_stream(request, target, '/pl.api.vizierpb.VizierService/ExecuteScript',
            src_dot_api_dot_public_dot_vizierapipb_dot_vizierapi__pb2.ExecuteScriptRequest.SerializeToString,
            src_dot_api_dot_public_dot_vizierapipb_dot_vizierapi__pb2.ExecuteScriptResponse.FromString,
            options, channel_credentials,
            insecure, call_credentials, compression, wait_for_ready, timeout, metadata)

    @staticmethod
    def HealthCheck(request,
            target,
            options=(),
            channel_credentials=None,
            call_credentials=None,
            insecure=False,
            compression=None,
            wait_for_ready=None,
            timeout=None,
            metadata=None):
        return grpc.experimental.unary_stream(request, target, '/pl.api.vizierpb.VizierService/HealthCheck',
            src_dot_api_dot_public_dot_vizierapipb_dot_vizierapi__pb2.HealthCheckRequest.SerializeToString,
            src_dot_api_dot_public_dot_vizierapipb_dot_vizierapi__pb2.HealthCheckResponse.FromString,
            options, channel_credentials,
            insecure, call_credentials, compression, wait_for_ready, timeout, metadata)
