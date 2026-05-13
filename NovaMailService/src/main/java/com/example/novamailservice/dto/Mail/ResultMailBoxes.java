package com.example.novamailservice.dto.Mail;

import com.example.novamailservice.entity.DomainNames.DomainNames;
import com.example.novamailservice.entity.Mail.MailBoxes;
import lombok.Data;

import java.time.LocalDateTime;

@Data
public class ResultMailBoxes {
    // 邮箱ID
    private Long mailBoxId;
    // 所属用户ID
    private Long userId;
    // 域名ID
    private Long domainNameId;
    // 邮箱地址
    private String localPart;
    // 是否启用(0: 禁用, 1: 启用, 2: 删除)
    private Integer isOpen;
    // 创建时间
    private LocalDateTime createTime;
    // 更新时间
    private LocalDateTime updateTime;
    private String MailBoxView;

    public static ResultMailBoxes MailBoxesToResultMailBoxes(MailBoxes mailBoxes, DomainNames domainNames) {
        ResultMailBoxes resultMailBox = new ResultMailBoxes();
        resultMailBox.setMailBoxId(mailBoxes.getMailBoxId());
        resultMailBox.setUserId(mailBoxes.getUserId());
        resultMailBox.setDomainNameId(mailBoxes.getDomainNameId());
        resultMailBox.setLocalPart(mailBoxes.getLocalPart());
        resultMailBox.setIsOpen(mailBoxes.getIsOpen());
        resultMailBox.setCreateTime(mailBoxes.getCreateTime());
        resultMailBox.setUpdateTime(mailBoxes.getUpdateTime());
        resultMailBox.setMailBoxView(mailBoxes.getLocalPart() + "@" + domainNames.getDomainName());
        return resultMailBox;
    }
}
