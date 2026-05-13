package com.example.novamailservice.util.NovaMailHub;

import org.springframework.stereotype.Component;

import java.util.Optional;
import java.util.concurrent.atomic.AtomicReference;

@Component
public class Session {
    private final AtomicReference<String> serviceId = new AtomicReference<>();

    public void setServiceId(String serviceId) {
        if (serviceId == null || serviceId.isBlank()) {
            throw new IllegalArgumentException("serviceId cannot be empty");
        }

        this.serviceId.set(serviceId);
    }

    public Optional<String> getServiceId() {
        return Optional.ofNullable(serviceId.get());
    }

    public String requireServiceId() {
        String value = serviceId.get();

        if (value == null || value.isBlank()) {
            throw new IllegalStateException("Hub serviceId has not been assigned yet");
        }

        return value;
    }

    public boolean hasServiceId() {
        String value = serviceId.get();
        return value != null && !value.isBlank();
    }
}