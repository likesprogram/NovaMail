package com.example.novamailservice.service.System;

import com.example.novamailservice.dto.Result.ResponseResult;
import com.example.novamailservice.dto.System.SystemServiceMonitorInfo;

import java.util.List;

public interface SystemServiceMonitorService {
    ResponseResult<List<SystemServiceMonitorInfo>> getServiceMonitorList();

    List<SystemServiceMonitorInfo> refreshStaleServices(long staleMillis);
}
