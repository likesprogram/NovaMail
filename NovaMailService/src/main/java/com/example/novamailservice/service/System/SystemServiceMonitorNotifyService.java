package com.example.novamailservice.service.System;

import com.example.novamailservice.dto.System.SystemServiceMonitorInfo;

public interface SystemServiceMonitorNotifyService {
    void notifyServiceChanged(SystemServiceMonitorInfo service);
}
