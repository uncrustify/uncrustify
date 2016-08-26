// This kind of thing is still a problem:

public class Class
{
    public override MyNodeType NodeType
    {
        get
        {
            if (entity != null)
                return entity.ReadState == ReadState.Initial ?
                       source.NodeType :
                       entity.EOF ? MyNodeType.EndEntity:
                       entity.NodeType;
            else
                return source.NodeType;
        }
    }
}

// The second ? should have a space before it.
