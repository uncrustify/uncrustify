void Win::check(QString id)
{
    if(names.contains(id + QString("_tst")) or names.contains(id + QString("_prd"))) {
	true;
    }
    return;
}