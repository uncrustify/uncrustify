void a()
{
	if((tmp == nullptr) ||
	   ((tmp->GetType() != CT_NUMBER) &&
	    (tmp->GetType() != CT_SIZEOF) &&
	    !(tmp->GetFlags() & (PCF_IN_STRUCT | PCF_IN_CLASS))) ||
	   (tmp->GetType() == CT_NEWLINE)
	   )
	{
		next->SetType(CT_LABEL_COLON);
	}
	else if ((tmp == nullptr) ||
	         ((tmp->GetType() != CT_NUMBER) &&
	          (tmp->GetType() != CT_SIZEOF) &&
	          !(tmp->GetFlags() & (PCF_IN_STRUCT | PCF_IN_CLASS))) ||
	         (tmp->GetType() == CT_NEWLINE)
	         )
	{
		next->SetType(CT_LABEL_COLON);
	}


	if                 ((tmp == nullptr) ||
	                    ((tmp->GetType() != CT_NUMBER) &&
	                     (tmp->GetType() != CT_SIZEOF) &&
	                     !(tmp->GetFlags() & (PCF_IN_STRUCT | PCF_IN_CLASS))) ||
	                    (tmp->GetType() == CT_NEWLINE)
	                    )
	{
		next->SetType(CT_LABEL_COLON);
	}

	if                 ((tmp == nullptr) ||
	                    ((tmp->GetType() != CT_NUMBER) &&
	                     (tmp->GetType() != CT_SIZEOF) &&
	                     !(tmp->GetFlags() & (PCF_IN_STRUCT | PCF_IN_CLASS))) ||
	                    (tmp->GetType() == CT_NEWLINE)
	                    )
	{
		next->SetType(CT_LABEL_COLON);
	}
}
