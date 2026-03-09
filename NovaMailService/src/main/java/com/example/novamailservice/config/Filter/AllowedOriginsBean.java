package com.example.novamailservice.config.Filter;

import lombok.Data;

import java.util.HashSet;
import java.util.Set;

@Data
public class AllowedOriginsBean {
    private Set<String> origin;

    public void addOne(String url) {
        if (this.getOrigin() == null) {
            this.origin = new HashSet<>();
        }
        this.getOrigin().add(url);
    }

    public void addInBatch(String[] urls) {
        for (String one : urls) {
            this.addOne(one);
        }
    }
}
