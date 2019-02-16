if (YES)
{
   NSString *sqlStr = [NSString stringWithFormat:@"INSERT INTO %@ (%@ , %@) VALUES
                              ('%@','%@')"
                       , ContactsRemark_Table
                       , ContactsRemark_FollowId
                       , ContactsRemark_MarkName
                       , followId
                       , markName
                      ];
}
