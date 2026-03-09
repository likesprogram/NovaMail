export const EmailActions = {
  None: 0,
  Edit: 1 << 0,
  Revoke: 1 << 1,
  Delete: 1 << 2,
  ReplyAll: 1 << 3,
  Forward: 1 << 4,
  NotSpam: 1 << 5,
  NotAd: 1 << 6,
  MarkRead: 1 << 7,
  Mark: 1 << 8,
  MoveTo: 1 << 9,
  All: (1 << 10) - 1,
} as const;

export type EmailActions = typeof EmailActions[keyof typeof EmailActions];
