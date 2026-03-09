package com.example.novamailservice.config.JacksonConfig;

import com.fasterxml.jackson.core.JsonParser;
import com.fasterxml.jackson.databind.DeserializationContext;
import com.fasterxml.jackson.databind.JsonDeserializer;
import org.springframework.beans.factory.annotation.Autowired;

import java.io.IOException;
import java.time.LocalDateTime;
import java.time.format.DateTimeFormatter;

public class GlobalLocalDateTimeDeserializer extends JsonDeserializer<LocalDateTime> {
    private final DateTimeFormatter formatter;
    @Autowired
    public GlobalLocalDateTimeDeserializer(DateTimeFormatter formatter) {
        this.formatter = formatter;
    }
    @Override
    public LocalDateTime deserialize(JsonParser p, DeserializationContext ctxt) throws IOException {
        String text = p.getText();
        return (text == null || text.isBlank()) ? null : LocalDateTime.parse(text, formatter);
    }
}
