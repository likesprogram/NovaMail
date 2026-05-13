package com.example.novamailservice.util.MinIo;

import lombok.AllArgsConstructor;
import lombok.Data;

import java.time.LocalDateTime;
import java.time.ZonedDateTime;

@Data
@AllArgsConstructor
public class FileInfo {
    private String objectName;
    private LocalDateTime lastModified;
    private long size;
    private String etag;

    public FileInfo(String objectName, ZonedDateTime lastModified, long size, String etag) {
        this.objectName = objectName;
        this.lastModified = lastModified.toLocalDateTime();
        this.size = size;
        this.etag = etag;
    }
}
