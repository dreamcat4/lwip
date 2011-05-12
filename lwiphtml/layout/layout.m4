m4_define(PAGEWIDTH, 100%)
m4_define(MENUWIDTH, 160)
m4_define(NEWSWIDTH, 160)
m4_define(BORDERWIDTH, 2)
m4_define(BOXWIDTH, 16)

m4_define(BORDER_START,
<table width="100%" cellspacing="0" cellpadding="0" border="0"><tr>
<td width="BORDERWIDTH" bgcolor="BORDERCOLOR"> </td>
<td bgcolor="BORDERCOLOR"> </td>
<td width="BORDERWIDTH" bgcolor="BORDERCOLOR"> </td>
</tr><tr>
<td width="BORDERWIDTH" bgcolor="BORDERCOLOR"> </td>
<td  bgcolor="TITLEBGCOLOR">
<table cellpadding="0"><tr><td align="center">
<b><font size="-1">$1</font></b><br>
</td></tr></table>
</td>
<td width="BORDERWIDTH" bgcolor="BORDERCOLOR"> </td>
</tr><tr>
<td width="BORDERWIDTH" bgcolor="BORDERCOLOR"> </td>
<td bgcolor="BORDERCOLOR"> </td>
<td width="BORDERWIDTH" bgcolor="BORDERCOLOR"> </td>
</tr><tr>
<td width="BORDERWIDTH" bgcolor="BORDERCOLOR"> </td>
<td>
<table cellpadding="$2"><tr><td>
)
m4_define(BORDER_END,
</td></tr></table>
</td>
<td width="BORDERWIDTH" bgcolor="BORDERCOLOR"> </td>
</tr><tr>
<td width="BORDERWIDTH" bgcolor="BORDERCOLOR"> </td>
<td bgcolor="BORDERCOLOR"> </td>
<td width="BORDERWIDTH" bgcolor="BORDERCOLOR"> </td>
</tr></table>
)

m4_define(RELATED_SITE,
<a href="$2"><b>$1</b></a><br>
<font size="-2">$3</font>
<br><br>
)


m4_define(UPDATED,
<p align="right">        
<font size="-1"><i>
Last updated: $1 (CEST)
</i></font>
</p>
)