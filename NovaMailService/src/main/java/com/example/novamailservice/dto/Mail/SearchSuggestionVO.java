package com.example.novamailservice.dto.Mail;

import lombok.Data;

@Data
public class SearchSuggestionVO {
    private Long id;
    private String type;
    private String text;
    private String subText;
    private Long mailId;
    private Long mappingId;
}
