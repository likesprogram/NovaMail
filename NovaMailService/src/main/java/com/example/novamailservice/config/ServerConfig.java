package com.example.novamailservice.config;

import com.example.novamailservice.config.Filter.AllowedOriginsBean;
import com.example.novamailservice.config.Filter.FilterProperties;
import com.example.novamailservice.config.Filter.LsnCorsFilter;
import com.example.novamailservice.util.IpRegion;
import com.example.novamailservice.util.JwtTokenProvider;
import lombok.Data;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;

@Data
@Configuration
public class ServerConfig {
    @Bean
    public AllowedOriginsBean getAllowedOriginsBean() {
        return new AllowedOriginsBean();
    }
    @Bean
    public LsnCorsFilter getLsnCorsFilter(FilterProperties filterProperties, AllowedOriginsBean corsOrigins, JwtTokenProvider jwtTokenProvider, IpRegion ipRegion) {
        return new LsnCorsFilter(filterProperties, corsOrigins, jwtTokenProvider, ipRegion);
    }
}
