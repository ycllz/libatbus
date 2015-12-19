#ifndef LIBATBUS_PROTOCOL_DESC_H_
#define LIBATBUS_PROTOCOL_DESC_H_

#pragma once

#include <cstddef>
#include <stdint.h>
#include <msgpack.hpp>

enum ATBUS_PROTOCOL_CMD {
    ATBUS_CMD_INVALID = 0,

    //  数据协议
    ATBUS_CMD_DATA_TRANSFORM_REQ = 1,
    ATBUS_CMD_DATA_TRANSFORM_RSP,
    ATBUS_CMD_CUSTOM_CMD_REQ,

    // 节点控制协议
    ATBUS_CMD_NODE_SYNC_REQ = 9,
    ATBUS_CMD_NODE_SYNC_RSP,
    ATBUS_CMD_NODE_REG_REQ,
    ATBUS_CMD_NODE_REG_RSP,
    ATBUS_CMD_NODE_CONN_SYN,
    ATBUS_CMD_NODE_PING,
    ATBUS_CMD_NODE_PONG,

    ATBUS_CMD_MAX
};

MSGPACK_ADD_ENUM(ATBUS_PROTOCOL_CMD);

namespace atbus {
    namespace protocol {
#ifndef ATBUS_MACRO_BUSID_TYPE
#define ATBUS_MACRO_BUSID_TYPE uint64_t
#endif

        struct bin_data_block {
            const void* ptr;
            size_t size;
        };

        struct custom_command_data {
            ATBUS_MACRO_BUSID_TYPE from;                // ID: 0
            std::vector<bin_data_block> commands;       // ID: 1

            custom_command_data(): from(0) {}

            MSGPACK_DEFINE(from, commands);
        };

        struct forward_data {
            ATBUS_MACRO_BUSID_TYPE from;                // ID: 0
            ATBUS_MACRO_BUSID_TYPE to;                  // ID: 1
            std::vector<ATBUS_MACRO_BUSID_TYPE> router; // ID: 2
            bin_data_block content;                     // ID: 3

            forward_data(): from(0), to(0) {
                content.size = 0;
                content.ptr = NULL;
            }

            MSGPACK_DEFINE(from, to, router, content);
        };

        struct channel_data {
            std::string address;                        // ID: 0
            MSGPACK_DEFINE(address);
        };

        struct node_data {
            ATBUS_MACRO_BUSID_TYPE bus_id;              // ID: 0
            bool overwrite;                             // ID: 1
            bool has_global_tree;                       // ID: 2
            ATBUS_MACRO_BUSID_TYPE children_id_mask;
            std::vector<node_data> children;

            node_data() : bus_id(0), overwrite(false), has_global_tree(false), children_id_mask(0){}

            MSGPACK_DEFINE(bus_id, overwrite, has_global_tree, children_id_mask, children);
        };

        struct node_tree {
            std::vector<node_data> nodes;               // ID: 0

            MSGPACK_DEFINE(nodes);
        };

        struct ping_data {
            int64_t time_point;                         // ID: 0

            ping_data(): time_point(0) {}

            MSGPACK_DEFINE(time_point);
        };

        struct reg_data {
            ATBUS_MACRO_BUSID_TYPE bus_id;          // ID: 0
            int32_t pid;                            // ID: 1
            std::string hostname;                   // ID: 2
            std::vector<channel_data> channels;     // ID: 3
            uint32_t children_id_mask;              // ID: 4
            bool has_global_tree;                   // ID: 5
            

            reg_data():bus_id(0), pid(0), children_id_mask(0), has_global_tree(false) {}

            MSGPACK_DEFINE(bus_id, pid, hostname, channels, children_id_mask, has_global_tree);
        };

        struct conn_data {
            channel_data address;                   // ID: 0

            MSGPACK_DEFINE(address);
        };

        class msg_body {
        public:
            forward_data* forward;
            node_tree* sync;
            ping_data* ping;
            reg_data* reg;
            conn_data* conn;
            custom_command_data* custom;

            msg_body(): forward(NULL), sync(NULL), ping(NULL), reg(NULL), conn(NULL), custom(NULL) {}
            ~msg_body() {
                if (NULL != forward) {
                    delete forward;
                }

                if (NULL != sync) {
                    delete sync;
                }

                if (NULL != ping) {
                    delete ping;
                }

                if (NULL != reg) {
                    delete reg;
                }

                if (NULL != conn) {
                    delete conn;
                }

                if (NULL != custom) {
                    delete custom;
                }
            }

            template<typename TPtr> 
            TPtr* make_body(TPtr*& p) {
                if (NULL != p) {
                    return p;
                }

                return p = new TPtr();
            }

            forward_data* make_forward(ATBUS_MACRO_BUSID_TYPE from, ATBUS_MACRO_BUSID_TYPE to, const void* buffer, size_t s) {
                forward_data* ret = make_body(forward);
                if (NULL == ret) {
                    return ret;
                }

                ret->from = from;
                ret->to = to;
                ret->content.ptr = buffer;
                ret->content.size = s;
                return ret;
            }

        private:
            msg_body(const msg_body&);
            msg_body& operator=(const msg_body&);
        };

        struct msg_head {
            ATBUS_PROTOCOL_CMD cmd;                 // ID: 0
            int32_t type;                           // ID: 1
            int32_t ret;                            // ID: 2
            uint32_t sequence;                      // ID: 3
            ATBUS_MACRO_BUSID_TYPE src_bus_id;      // ID: 4

            msg_head(): cmd(ATBUS_CMD_INVALID), type(0), ret(0), sequence(0) {}

            MSGPACK_DEFINE(cmd, type, ret, sequence, src_bus_id);
        };

        struct msg {
            msg_head head;              // map.key = 1
            msg_body body;              // map.key = 2

            void init(ATBUS_MACRO_BUSID_TYPE src_bus_id, ATBUS_PROTOCOL_CMD cmd, int32_t type, int32_t ret, uint32_t seq) {
                head.cmd = cmd;
                head.type = type;
                head.ret = ret;
                head.sequence = seq;
                head.src_bus_id = src_bus_id;
            }
        };
    }
}


// User defined class template specialization
namespace msgpack {
    MSGPACK_API_VERSION_NAMESPACE(MSGPACK_DEFAULT_API_NS) {
        namespace adaptor {

            template<>
            struct convert<atbus::protocol::bin_data_block> {
                msgpack::object const& operator()(msgpack::object const& o, atbus::protocol::bin_data_block& v) const {
                    if (o.type != msgpack::type::BIN) throw msgpack::type_error();

                    v.ptr = o.via.bin.ptr;
                    v.size = o.via.bin.size;
                    return o;
                }
            };

            template<>
            struct pack<atbus::protocol::bin_data_block> {
                template <typename Stream>
                packer<Stream>& operator()(msgpack::packer<Stream>& o, atbus::protocol::bin_data_block const& v) const {
                    o.pack_bin(static_cast<uint32_t>(v.size));
                    o.pack_bin_body(reinterpret_cast<const char*>(v.ptr), static_cast<uint32_t>(v.size));
                    return o;
                }
            };

            template <>
            struct object_with_zone<atbus::protocol::bin_data_block> {
                void operator()(msgpack::object::with_zone& o, atbus::protocol::bin_data_block const& v) const {
                    o.type = type::BIN;
                    o.via.bin.size =  static_cast<uint32_t>(v.size);
                    o.via.bin.ptr =  reinterpret_cast<const char*>(v.ptr);
                }
            };

            template<>
            struct convert<atbus::protocol::msg> {
                msgpack::object const& operator()(msgpack::object const& o, atbus::protocol::msg& v) const {
                    if (o.type != msgpack::type::MAP) throw msgpack::type_error();
                    msgpack::object body_obj;
                    // just like protobuf buffer
                    for (uint32_t i = 0; i < o.via.map.size; ++ i) {
                        if (o.via.map.ptr[i].key.via.u64 == 1) {
                            o.via.map.ptr[i].val.convert(v.head);
                        } else if (o.via.map.ptr[i].key.via.u64 == 2) {
                            body_obj = o.via.map.ptr[i].val;
                        }
                    }


                    // unpack body using head.cmd
                    if(!body_obj.is_nil()) {
                        switch(v.head.cmd) {

                        case ATBUS_CMD_DATA_TRANSFORM_REQ:
                        case ATBUS_CMD_DATA_TRANSFORM_RSP: {
                            body_obj.convert(*v.body.make_body(v.body.forward));
                            break;
                        }

                        case ATBUS_CMD_NODE_SYNC_RSP: {
                            body_obj.convert(*v.body.make_body(v.body.sync));
                            break;
                        }

                        case ATBUS_CMD_NODE_REG_REQ:
                        case ATBUS_CMD_NODE_REG_RSP: {
                            body_obj.convert(*v.body.make_body(v.body.reg));
                            break;
                        }

                        case ATBUS_CMD_NODE_CONN_SYN: {
                            body_obj.convert(*v.body.make_body(v.body.conn));
                            break;
                        }

                        case ATBUS_CMD_NODE_PING:
                        case ATBUS_CMD_NODE_PONG: {
                            body_obj.convert(*v.body.make_body(v.body.ping));
                            break;
                        }

                        default: { // invalid cmd
                            break;
                        }
                        }
                    }

                    return o;
                }
            };

            template<>
            struct pack<atbus::protocol::msg> {
                template <typename Stream>
                packer<Stream>& operator()(msgpack::packer<Stream>& o, atbus::protocol::msg const& v) const {
                    // packing member variables as an map.
                    o.pack_map(2);
                    o.pack(1);
                    o.pack(v.head);
                    
                    // pack body using head.cmd
                    o.pack(2);
                    switch (v.head.cmd) {

                    case ATBUS_CMD_DATA_TRANSFORM_REQ:
                    case ATBUS_CMD_DATA_TRANSFORM_RSP: {
                        if (NULL == v.body.forward) {
                            o.pack_nil();
                        } else {
                            o.pack(*v.body.forward);
                        }
                        break;
                    }

                    case ATBUS_CMD_NODE_SYNC_RSP: {
                        if (NULL == v.body.sync) {
                            o.pack_nil();
                        } else {
                            o.pack(*v.body.sync);
                        }
                        break;
                    }

                    case ATBUS_CMD_NODE_REG_REQ:
                    case ATBUS_CMD_NODE_REG_RSP: {
                        if (NULL == v.body.reg) {
                            o.pack_nil();
                        } else {
                            o.pack(*v.body.reg);
                        }
                        break;
                    }

                    case ATBUS_CMD_NODE_CONN_SYN: {
                        if (NULL == v.body.sync) {
                            o.pack_nil();
                        } else {
                            o.pack(*v.body.sync);
                        }
                        break;
                    }

                    case ATBUS_CMD_NODE_PING:
                    case ATBUS_CMD_NODE_PONG: {
                        if (NULL == v.body.ping) {
                            o.pack_nil();
                        } else {
                            o.pack(*v.body.ping);
                        }
                        break;
                    }

                    default: { // invalid cmd
                        break;
                    }
                    }
                    return o;
                }
            };

            template <>
            struct object_with_zone<atbus::protocol::msg> {
                void operator()(msgpack::object::with_zone& o, atbus::protocol::msg const& v) const {
                    o.type = type::MAP;
                    o.via.map.size = 2;
                    o.via.map.ptr = static_cast<msgpack::object_kv*>(
                        o.zone.allocate_align(sizeof(msgpack::object_kv) * o.via.map.size));

                    o.via.map.ptr[0] = msgpack::object_kv();
                    o.via.map.ptr[0].key = msgpack::object(1);
                    v.head.msgpack_object(&o.via.map.ptr[0].val, o.zone);

                    // pack body using head.cmd
                    o.via.map.ptr[1].key = msgpack::object(1);
                    switch (v.head.cmd) {

                    case ATBUS_CMD_DATA_TRANSFORM_REQ:
                    case ATBUS_CMD_DATA_TRANSFORM_RSP: {
                        if (NULL == v.body.forward) {
                            o.via.map.ptr[1].val = msgpack::object();
                        } else {
                            v.body.forward->msgpack_object(&o.via.map.ptr[1].val, o.zone);
                        }
                        break;
                    }

                    case ATBUS_CMD_NODE_SYNC_RSP: {
                        if (NULL == v.body.sync) {
                            o.via.map.ptr[1].val = msgpack::object();
                        } else {
                            v.body.sync->msgpack_object(&o.via.map.ptr[1].val, o.zone);
                        }
                        break;
                    }

                    case ATBUS_CMD_NODE_REG_REQ:
                    case ATBUS_CMD_NODE_REG_RSP: {
                        if (NULL == v.body.reg) {
                            o.via.map.ptr[1].val = msgpack::object();
                        } else {
                            v.body.reg->msgpack_object(&o.via.map.ptr[1].val, o.zone);
                        }
                        break;
                    }

                    case ATBUS_CMD_NODE_CONN_SYN: {
                        if (NULL == v.body.sync) {
                            o.via.map.ptr[1].val = msgpack::object();
                        } else {
                            v.body.sync->msgpack_object(&o.via.map.ptr[1].val, o.zone);
                        }
                        break;
                    }

                    case ATBUS_CMD_NODE_PING:
                    case ATBUS_CMD_NODE_PONG: {
                        if (NULL == v.body.ping) {
                            o.via.map.ptr[1].val = msgpack::object();
                        } else {
                            v.body.ping->msgpack_object(&o.via.map.ptr[1].val, o.zone);
                        }
                        break;
                    }

                    default: { // invalid cmd
                        break;
                    }
                    }
                }
            };

        } // namespace adaptor
    } // MSGPACK_API_VERSION_NAMESPACE(MSGPACK_DEFAULT_API_NS)
} // namespace msgpack

#endif  // LIBATBUS_PROTOCOL_DESC_H_
