package com.example.novamailservice.util.NovaMailHub.Message;

import io.netty.buffer.ByteBuf;
import io.netty.channel.ChannelHandlerContext;
import io.netty.handler.codec.MessageToByteEncoder;

public class RpcMessageEncoder extends MessageToByteEncoder<RpcMessage> {
    @Override
    protected void encode(ChannelHandlerContext ctx, RpcMessage msg, ByteBuf out) {
        RpcCodec.writeToByteBuf(msg, out);
    }
}