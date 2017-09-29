// { dg-additional-sources "imports/gdc231a.d" }
module gdc231;

import imports.gdc231a;

class Range : Widget
{
    override void* getStruct()
    {
        return null;
    }
}

void main()
{
}
