package com.example.novamailservice.config.JacksonConfig;

import com.fasterxml.jackson.core.JsonGenerator;
import com.fasterxml.jackson.databind.JsonSerializer;
import com.fasterxml.jackson.databind.SerializerProvider;
import org.springframework.beans.factory.annotation.Autowired;

import java.io.IOException;
import java.time.LocalDateTime;
import java.time.format.DateTimeFormatter;

public class GlobalLocalDateTimeSerializer extends JsonSerializer<LocalDateTime> {
    private final DateTimeFormatter formatter;
    @Autowired
    public GlobalLocalDateTimeSerializer(DateTimeFormatter formatter) {
        this.formatter = formatter;
    }
    @Override
    public void serialize(LocalDateTime value, JsonGenerator gen, SerializerProvider serializers) throws IOException {
        gen.writeString(value != null ? value.format(formatter) : null);
    }
}
