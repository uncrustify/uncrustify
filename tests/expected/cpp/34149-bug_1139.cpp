void a()
{
	if((tmp == nullptr) ||
	   ((tmp->type != CT_NUMBER) &&
	    (tmp->type != CT_SIZEOF) &&
	    !(tmp->flags & (PCF_IN_STRUCT | PCF_IN_CLASS))) ||
	   (tmp->type == CT_NEWLINE))
	{
		set_chunk_type(next, CT_LABEL_COLON);
	}
	else if ((tmp == nullptr) ||
	         ((tmp->type != CT_NUMBER) &&
	          (tmp->type != CT_SIZEOF) &&
	          !(tmp->flags & (PCF_IN_STRUCT | PCF_IN_CLASS))) ||
	         (tmp->type == CT_NEWLINE))
	{
		set_chunk_type(next, CT_LABEL_COLON);
	}


	if                 ((tmp == nullptr) ||
	                    ((tmp->type != CT_NUMBER) &&
	                     (tmp->type != CT_SIZEOF) &&
	                     !(tmp->flags & (PCF_IN_STRUCT | PCF_IN_CLASS))) ||
	                    (tmp->type == CT_NEWLINE))
	{
		set_chunk_type(next, CT_LABEL_COLON);
	}

	if                 ((tmp == nullptr) ||
	                    ((tmp->type != CT_NUMBER) &&
	                     (tmp->type != CT_SIZEOF) &&
	                     !(tmp->flags & (PCF_IN_STRUCT | PCF_IN_CLASS))) ||
	                    (tmp->type == CT_NEWLINE))
	{
		set_chunk_type(next, CT_LABEL_COLON);
	}
}
