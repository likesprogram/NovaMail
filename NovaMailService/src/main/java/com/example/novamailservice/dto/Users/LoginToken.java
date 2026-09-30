package com.example.novamailservice.dto.Users;

import lombok.Data;

@Data
public class LoginToken {
    private String tokenType;
    private String accessToken;
    private String refreshToken;
    private long accessTokenExpiresIn;
    private long refreshTokenExpiresIn;

    public LoginToken(String accessToken, String refreshToken, long accessTokenExpiresIn, long refreshTokenExpiresIn) {
        this.tokenType = "Bearer";
        this.accessToken = accessToken;
        this.refreshToken = refreshToken;
        this.accessTokenExpiresIn = accessTokenExpiresIn;
        this.refreshTokenExpiresIn = refreshTokenExpiresIn;
    }
}
