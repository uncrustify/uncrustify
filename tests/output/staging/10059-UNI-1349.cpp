// incorrect

typedef ::GlobalType GlobalType;

// Using the sp_before_dc and sp_after_dc options with 'remove' causes the above due to global scope. Need to fix Uncrustify to only do this removal only for CT_WORD and not keywords (or anything else, probably).

// Also make sure this is handled right:

// correct

void foo()
{
    return (int)::Bar();
    // ... --> ...
    return (int)::Bar();
}

// Second line removes the space, which we still want in the new setup. (Though really that removal should depend on casting operator spacing but whatev.)

// correct

typedef void (Goo::*Boo)(const TYPE &data, ULONG index, LONG minimum, LONG maximum);
// ... --> ...
typedef void (Goo::*Boo)(const TYPE &data, ULONG index, LONG minimum, LONG maximum);

// incorrect

struct ClassImpl : public ::Class {};
// ... --> ...
struct ClassImpl : public ::Class {};

// incorrect

friend class ::Clasz;
// ... --> ...
friend class ::Clasz;

// correct

void bar()
{
    vector<Texture2D*>::iterator found = find(uniqueTextures.begin(), uniqueTextures.end(), textures[i]);
    // ... --> ...
    vector<Texture2D*>::iterator found = find(uniqueTextures.begin(), uniqueTextures.end(), textures[i]);
}

// incorrect

using namespace ::comp;
using namespace ::comp::run;
// ... --> ...
using namespace ::comp;
using namespace ::comp::run;

// incorrect

class ClassDummy : public ::comp::run::Class
{
};
// ... --> ...
class ClassDummy : public ::comp::run::Class
{
};

// Starting to think this may be too hard, because what precedes the :: may be complex. What if it's just a > operator in a comparison expression against a global?
