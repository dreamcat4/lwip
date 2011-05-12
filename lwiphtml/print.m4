m4_changecom(`/*', `*/')

m4_include(html/FILE.m4)
m4_ifdef(`CATEGORY',
`m4_include(html/CATEGORY.m4)'
,)
m4_include(layout/layout.m4)

<h2>OVERALL_TITLE - OVERALL_SUBTITLE</h2>

<h3>TITLE</h3>

URL: <tt>http://www.sics.se/~adam/lwip/FILE.html</tt>
<br>

m4_include(html/FILE.html)