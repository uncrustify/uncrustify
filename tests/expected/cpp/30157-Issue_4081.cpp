class RenderStyle : public Shared<RenderStyle>
{
  union
  {
    struct
    {
      E    _empty_cells         : 1;
      E2   _caption_side        : 2;
      E34  _list_style_type     : 6;
      EL77 _list_style_position : 1;
    } f;
    TQ_UINT64 _iflags;
  };
};
