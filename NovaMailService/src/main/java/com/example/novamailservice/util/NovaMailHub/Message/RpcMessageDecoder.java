package com.example.novamailservice.util.NovaMailHub.Message;

import io.netty.channel.ChannelHandlerContext;
import io.netty.handler.codec.ByteToMessageDecoder;

import java.util.List;

public class RpcMessageDecoder extends ByteToMessageDecoder {
    private final long maxBodySize;

    public RpcMessageDecoder() {
        this(0);
    }

    public RpcMessageDecoder(long maxBodySize) {
        this.maxBodySize = maxBodySize;
    }

    @Override
    protected void decode(ChannelHandlerContext ctx, io.netty.buffer.ByteBuf in, List<Object> out) {
        RpcParseResult result = RpcCodec.tryParse(in, maxBodySize);

        if (result.isIncomplete()) return;

        if (!result.isSuccess()) {
            ctx.close();
            throw new IllegalStateException(result.getErrorMessage());
        }
        out.add(result.getMessage());
    }
}