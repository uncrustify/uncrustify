void ABC()
{
bool bb = true;
bool ccc = true;
char x;
// simple: E1 ? E2 : E3
x =
   bb  ?
               'T'
                  : 'F';
// simple_R: E1 ? E2 : E1r ? E2r : E3r
x =
   bb  ?
               'T'
                  : ccc ?
  'r'
      : 'F';
 simple_L: E1 ? E1l ? E2l : E3l : E3
x =
   bb  ?
                  (ccc) ?
               't'
  : 'l'
      : 'F';
// CDanU E1 ? E1l ? E2ll : E1ll ? E3ll : E3l : E3
x =
   (bb) ?
       (is_newline_tmp) ? 'n'
            : (is_comment_tmp) ? 'c'
               : 'o'
                  : '-';


//CDanU E1 ?  E1l              ? E2ll : E1ll              ? E3ll : E3l : E3
// x = (y) ?  _is_newline(tmp) ? 'n'  :  _is_comment(tmp) ? 'c'  : 'o' : '-';
//                                     |E1______________ c E2_ C E3_|
//           |E1______________ b E2_ B E3___________________________|
//    |E1_ a E2_____________________________________________________ A E3|
}
