// Bug 280

struct RBNode
{
    RBNode* _parent;

    @property left(RBNode*)
    {
        _parent = &this;
    }
}

class RedBlackTree
{
    RBNode* _end;
    RBNode* _begin;

    this(int[] elems...)
    {
        _end = new RBNode;

        foreach (e; elems)
        {
            _end.left = _begin;
        }
    }
}

__gshared s = new RedBlackTree('h');
