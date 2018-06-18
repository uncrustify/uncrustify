[[HRNewsService sharedInstance] unregisterPushToken: data
                                            success:^{
                                                        self.notificationsEnabled = NO;
                                                        if (data.param)
                                                        {
                                                            self.loudNotifications = YES;
                                                        }
                                                    }
                                               fail:^{
                                                        self.notificationsEnabled = NO;
                                                        if (data.param)
                                                        {
                                                            self.loudNotifications = YES;
                                                        }
                                                    }
];
