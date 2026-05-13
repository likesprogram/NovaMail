package com.example.novamailservice.util.NovaMailHub.Message;

import java.security.SecureRandom;
import java.time.Instant;
import java.util.concurrent.atomic.AtomicLong;

public final class RequestIdGenerator {
    private static final long CUSTOM_EPOCH = 1704067200000L; // 2024-01-01 00:00:00 UTC
    private static final int SEQUENCE_BITS = 20;
    private static final long MAX_SEQUENCE = (1L << SEQUENCE_BITS) - 1;

    private static final AtomicLong LAST_ID = new AtomicLong(initialValue());

    private RequestIdGenerator() {}

    public static long nextId() {
        while (true) {
            long current = LAST_ID.get();

            long nowMillis = Instant.now().toEpochMilli();
            long timestampPart = Math.max(0, nowMillis - CUSTOM_EPOCH) << SEQUENCE_BITS;

            long next = Math.max(current + 1, timestampPart);

            if (LAST_ID.compareAndSet(current, next)) return next;
        }
    }

    private static long initialValue() {
        long nowMillis = Instant.now().toEpochMilli();
        long timestampPart = Math.max(0, nowMillis - CUSTOM_EPOCH) << SEQUENCE_BITS;

        long randomPart = new SecureRandom().nextInt(1 << 12);

        return timestampPart | randomPart;
    }
}