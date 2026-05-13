package com.example.novamailservice.config.Filter;

import java.util.HashSet;
import java.util.Set;

public class AllowedOriginsBean {
    private final Set<String> origin = new HashSet<>();
    private final Set<String> whiteList = new HashSet<>();

    // Origin
    public void addOrigin(String url) {

        this.origin.add(url);
    }
    public void addOrigins(String[] urls) {
        if (urls == null) return;
        for (String one : urls) addOrigin(one);
    }
    public boolean containsOrigin(String url) {
        return origin.contains(url);
    }

    // WhiteList
    public void addWhite(String path) {
        this.whiteList.add(path);
    }
    public void addWhiteBatch(String[] paths) {
        if (paths == null) return;
        for (String one : paths) addWhite(one);
    }
    public boolean containsWhite(String uri) {
        if (uri == null) return false;
        return whiteList.stream().anyMatch(uri::startsWith);
    }
}
