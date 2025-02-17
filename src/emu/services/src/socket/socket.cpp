/*
 * Copyright (c) 2021 EKA2L1 Team
 * 
 * This file is part of EKA2L1 project.
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <services/socket/server.h>
#include <services/socket/socket.h>

#include <utils/err.h>

namespace eka2l1::epoc::socket {
    std::size_t socket::get_option(const std::uint32_t option_id, const std::uint32_t option_family,
        std::uint8_t *buffer, const std::size_t avail_size) {
        LOG_ERROR(SERVICE_ESOCK, "Unhandled base option family {} (id {})", option_family, option_id);
        return 0;
    }

    bool socket::set_option(const std::uint32_t option_id, const std::uint32_t option_family,
        std::uint8_t *buffer, const std::size_t avail_size) {
        LOG_ERROR(SERVICE_ESOCK, "Unhandled base option family {} (id {})", option_family, option_id);
        return false;
    }

    std::int32_t socket::bind(const std::uint8_t *sockaddr_buffer, const std::size_t available_size) {
        LOG_ERROR(SERVICE_ESOCK, "Binding socket unimplemented");
        return epoc::error_permission_denied;
    }

    void socket::ioctl(const std::uint32_t command, epoc::notify_info &complete_info, std::uint8_t *buffer,
        const std::size_t available_size, const std::size_t max_buffer_size, const std::uint32_t level) {
        LOG_ERROR(SERVICE_ESOCK, "Unhandled base IOCTL (command={})", command);
    }

    void socket::send(const std::uint8_t *data, const std::size_t data_size, std::size_t *sent_size, const std::uint8_t *sockaddr_buffer,
        const std::size_t sockaddr_buffer_size, const std::uint32_t flags, epoc::notify_info &complete_info) {
        LOG_ERROR(SERVICE_ESOCK, "Sending data to socket unimplemented!");
    }

    socket_socket::socket_socket(socket_client_session *parent, std::unique_ptr<socket> &sock)
        : socket_subsession(parent)
        , sock_(std::move(sock)) {
    }

    struct oldarch_option_description {
        std::uint32_t id_;
        eka2l1::ptr<epoc::desc8> data_;
        std::uint32_t data_size_;
    };

    void socket_socket::get_option(service::ipc_context *ctx) {
        std::optional<std::uint32_t> option_name;
        std::optional<std::uint32_t> option_fam;

        std::uint8_t *dest_buffer = nullptr;
        std::size_t max_size = 0;

        kernel::process *caller = ctx->msg->own_thr->owning_process();
        epoc::desc8 *custom_buf = nullptr;

        if (parent_->is_oldarch()) {
            auto desc_data = ctx->get_argument_data_from_descriptor<oldarch_option_description>(0);
            if (!desc_data.has_value()) {
                ctx->complete(epoc::error_argument);
                return;
            }

            option_name = desc_data->id_;
            max_size = desc_data->data_size_;
            option_fam = ctx->get_argument_value<std::uint32_t>(1);

            epoc::desc8 *data_des_real = desc_data->data_.get(caller);
            if (!data_des_real) {
                ctx->complete(epoc::error_argument);
                return;
            }

            dest_buffer = reinterpret_cast<std::uint8_t *>(data_des_real->get_pointer(caller));
            custom_buf = data_des_real;
        } else {
            option_name = ctx->get_argument_value<std::uint32_t>(0);
            option_fam = ctx->get_argument_value<std::uint32_t>(2);

            dest_buffer = ctx->get_descriptor_argument_ptr(1);
            max_size = ctx->get_argument_max_data_size(1);
        }

        if (!option_name || !option_fam || !dest_buffer) {
            ctx->complete(epoc::error_argument);
            return;
        }

        const std::size_t res = sock_->get_option(option_name.value(), option_fam.value(), dest_buffer, max_size);
        if (res == static_cast<std::size_t>(-1)) {
            LOG_ERROR(SERVICE_ESOCK, "Fail to get value of socket option!");
            ctx->complete(epoc::error_general);

            return;
        }

        if (custom_buf) {
            custom_buf->set_length(caller, static_cast<std::uint32_t>(res));
        } else {
            ctx->set_descriptor_argument_length(1, static_cast<std::uint32_t>(res));
        }

        ctx->complete(epoc::error_none);
    }

    void socket_socket::set_option(service::ipc_context *ctx) {
        std::optional<std::uint32_t> option_name;
        std::optional<std::uint32_t> option_fam;

        std::uint8_t *source_buffer = nullptr;
        std::size_t max_size = 0;

        kernel::process *caller = ctx->msg->own_thr->owning_process();

        if (parent_->is_oldarch()) {
            auto desc_data = ctx->get_argument_data_from_descriptor<oldarch_option_description>(0);
            if (!desc_data.has_value()) {
                ctx->complete(epoc::error_argument);
                return;
            }

            option_name = desc_data->id_;
            max_size = desc_data->data_size_;
            option_fam = ctx->get_argument_value<std::uint32_t>(1);

            epoc::desc8 *data_des_real = desc_data->data_.get(caller);
            if (data_des_real) {
                source_buffer = reinterpret_cast<std::uint8_t *>(data_des_real->get_pointer(caller));
            }
        } else {
            option_name = ctx->get_argument_value<std::uint32_t>(0);
            option_fam = ctx->get_argument_value<std::uint32_t>(2);

            source_buffer = ctx->get_descriptor_argument_ptr(1);
            max_size = ctx->get_argument_max_data_size(1);
        }

        if (!option_name || !option_fam) {
            ctx->complete(epoc::error_argument);
            return;
        }

        const bool res = sock_->set_option(option_name.value(), option_fam.value(), source_buffer, max_size);
        if (!res) {
            LOG_ERROR(SERVICE_ESOCK, "Fail to set value of socket option!");
            ctx->complete(epoc::error_general);

            return;
        }

        ctx->complete(epoc::error_none);
    }

    void socket_socket::bind(service::ipc_context *ctx) {
        std::uint8_t *sockaddr_buffer = ctx->get_descriptor_argument_ptr(0);
        std::size_t sockaddr_buffer_size = ctx->get_argument_data_size(0);

        if (!sockaddr_buffer || !sockaddr_buffer_size) {
            ctx->complete(epoc::error_argument);
            return;
        }

        const std::int32_t bind_result = sock_->bind(sockaddr_buffer, sockaddr_buffer_size);
        ctx->complete(bind_result);
    }

    void socket_socket::write(service::ipc_context *ctx) {
        std::uint8_t *packet_buffer = ctx->get_descriptor_argument_ptr(2);
        std::size_t packet_size = ctx->get_argument_data_size(2);

        if (!packet_buffer || !packet_size) {
            ctx->complete(epoc::error_argument);
            return;
        }

        epoc::notify_info info(ctx->msg->request_sts, ctx->msg->own_thr);
        sock_->send(packet_buffer, packet_size, nullptr, nullptr, 0, 0, info);
    }

    void socket_socket::ioctl(service::ipc_context *ctx) {
        std::optional<std::uint32_t> command = ctx->get_argument_value<std::uint32_t>(0);
        std::optional<std::uint32_t> level = ctx->get_argument_value<std::uint32_t>(2);
        std::uint8_t *buffer = ctx->get_descriptor_argument_ptr(1);
        std::size_t buffer_size = ctx->get_argument_data_size(1);
        std::size_t max_buffer_size = ctx->get_argument_max_data_size(1);

        if (!command.has_value() || !level.has_value()) {
            ctx->complete(epoc::error_none);
            return;
        }

        epoc::notify_info info(ctx->msg->request_sts, ctx->msg->own_thr);
        sock_->ioctl(command.value(), info, buffer, buffer_size, max_buffer_size, level.value());
    }

    void socket_socket::close(service::ipc_context *ctx) {
        parent_->subsessions_.remove(id_);
        ctx->complete(epoc::error_none);
    }

    void socket_socket::dispatch(service::ipc_context *ctx) {
        if (parent_->is_oldarch()) {
            switch (ctx->msg->function) {
            case socket_old_so_set_opt:
                set_option(ctx);
                return;

            case socket_old_so_get_opt:
                get_option(ctx);
                return;

            case socket_old_so_close:
                close(ctx);
                return;

            default:
                break;
            }
        } else {
            switch (ctx->msg->function) {
            case socket_so_get_opt:
                get_option(ctx);
                return;

            case socket_so_close:
                close(ctx);
                return;

            case socket_so_bind:
                bind(ctx);
                return;

            case socket_so_write:
                write(ctx);
                return;

            case socket_so_ioctl:
                ioctl(ctx);
                return;

            default:
                break;
            }
        }

        LOG_ERROR(SERVICE_ESOCK, "Unimplemented socket opcode: {}", ctx->msg->function);
    }
}