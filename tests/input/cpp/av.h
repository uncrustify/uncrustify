/*
2) There seems to be a problem handling .h-files compared to .cpp-files.
The following problem only occurs in header-files, in source-files the
output is as desired.
*/

static inline void foo()
{
//BEFORE:
if (cond) callFunc();
// DESIRED:
if (cond) {
  callFunc();
}
// AFTER:
if (cond) {callFunc();}


/*
3) The spacing around pointer stars is not always maintained as desired.
*/
//BEFORE:
  Buffer<T>* buffer;
//AFTER:
  Buffer<T>*buffer;


/*
4) Inside of casts the types are not formatted as outside.
*/
//BEFORE:
T* t = dynamic_cast<T*>(obj);
//AFTER:
T* t = dynamic_cast<T *>(obj);

/*
5) Inside some template-stuff the spacing goes weird. Multiple spaces
are inserted, although the configuration (should) say otherwise.
*/
//BEFORE:
for (std::map<Key, Value*>::iterator it = map.begin(); it != map.end(); it++) {
   bar(it);
}
//AFTER:
for (std::map < Key, Value * > ::iterator it = map.begin(); it != map.end(); it++) {
   bar(it);
}
   }

