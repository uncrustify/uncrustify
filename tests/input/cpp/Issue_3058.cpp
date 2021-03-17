void main()
{
  switch( m_modOnHdReason )
  {
    case CenteredAutoFit:
        if( ww <= w && wh <= h ) {
            d.setRect((w - ww) / 2, (h - wh) / 2, ww, wh); // like Centered
            break;
        }
        break; // Just a comment

    case CenteredMaxpect: {
        d.setRect((w - ww) / 2, (h - wh) / 2, ww, wh);
        break;
    }
  }
}

