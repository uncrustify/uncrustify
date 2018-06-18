for(int f=0; f<(Element::nf)*2; f++)
{
	if (f%2==1) p = p-1;
	{
		this->pInterpolation[i]=p;
		this->cInterpolation[i]=0.;
		this->dofInterpolation[i]=e+f;
		this->coefInterpolation[i]=1.;
		i++;
		p++;
	}
}
