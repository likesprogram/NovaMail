package com.example.novamailservice.dto.Users;

import lombok.Data;

@Data
public class UpdatePassword {
    String oldPassword;
    String newPassword;
    String confirmPassword;
}
