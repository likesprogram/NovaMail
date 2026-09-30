package com.example.novamailservice;

import org.springframework.boot.SpringApplication;
import org.springframework.boot.autoconfigure.SpringBootApplication;
import org.springframework.boot.web.embedded.tomcat.TomcatServletWebServerFactory;
import org.springframework.boot.web.server.WebServerFactoryCustomizer;
import org.springframework.boot.web.servlet.ServletComponentScan;
import org.springframework.context.annotation.Bean;
import org.springframework.scheduling.annotation.EnableAsync;
import org.springframework.scheduling.annotation.EnableScheduling;
import org.springframework.transaction.annotation.EnableTransactionManagement;

@EnableTransactionManagement
@SpringBootApplication
@EnableAsync  // 启用异步执行
@EnableScheduling
@ServletComponentScan  // 启用 Servlet 组件扫描，自动注册 @WebFilter 注解的过滤器
public class NovaMailServiceApplication {
    public static void main(String[] args) {
        SpringApplication.run(NovaMailServiceApplication.class, args);
        //启动成功
        System.out.println("(♥◠‿◠)ﾉﾞ  启动成功   ლ(´ڡ`ლ)ﾞ  \n");
    }

    @Bean
    public WebServerFactoryCustomizer<TomcatServletWebServerFactory> tomcatCustomizer() {
        return factory -> {
            factory.addConnectorCustomizers(connector -> {
                // 允许在查询参数中使用特殊字符
                connector.setProperty("relaxedQueryChars", "|{}[]");
            });
        };
    }

}
