void Tst::test(Msg *message_p)
{
   switch (message_p)
   {
   case A:
   {
      const table *entry2 = findMsg(message_p);
      table       *entry3 = findMsg(message_p);
   }
   break;

   case B:
      const table *entry2 = findMsg(message_p);
      table       *entry3 = findMsg(message_p);
      break;

   default:
      break;
   }
}
