void main()
{
	if (true)
	{
		// Output the coordinates of the icon
		snprintf(&ret[strlen(ret)], ret_max - strlen(ret), "\nx=\"0px\"\ny=\"0px\"\n");
		snprintf(&ret[strlen(ret)], ret_max - strlen(ret), "width=\"%d\"\n", icon->icon_width);
		snprintf(&ret[strlen(ret)], ret_max - strlen(ret), "height=\"%d\"\n", icon->icon_height);
		snprintf(&ret[strlen(ret)], ret_max - strlen(ret), "viewBox=\"%lf %lf %lf %lf\"\n", icon->x,
		        icon->y, icon->width, icon->height);

		snprintf(&ret[strlen(ret)], ret_max - strlen(ret), icon->x, icon->y,
		        bar("viewBox=\"%lf %lf %lf %lf\"\n", icon->width), icon->height);

		snprintf(&ret[strlen(ret)], ret_max - strlen(ret), "viewBox=\"%lf %lf %lf %lf\"\n", icon->x,
		        icon->y, icon->width, icon->height);
		snprintf(&ret[strlen(ret)], ret_max - strlen(ret), "viewBox=\"%lf %lf %lf %lf\"\n",
		        foo(icon->x, icon->y, icon->width), icon->height);
		snprintf(&ret[strlen(ret)], ret_max - strlen(ret), icon->x, icon->y,
		        bar("viewBox=\"%lf %lf %lf %lf\"\n", icon->width), icon->height);
		snprintf(&ret[strlen(ret)], ret_max - foo(ret, par), icon->x, icon->y,
		        bar("viewBox=\"%lf %lf %lf %lf\"\n", icon->width), icon->height);

		create_a_reeeeeeeeeeeeeeeeeeeeeeeeeeeeally_long_identifier name(some_function(bar1 + bar2),
		        bar3, bar4);
		create_a_reeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeally_long_identifier name(some_function(
		        bar1 + bar2), bar3, bar4);

		abc.def.ghi = call_some_other_really_long_function.of_some_sort_of_very_very_very_loooooooooong(
		        some_long_parameter1, some_long_parameter2);
	}
}

#define FOO(bar) create_a_really_long_identifier name(some_function(bar1 + bar2), bar3, bar4);
