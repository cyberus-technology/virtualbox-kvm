<?xml version="1.0"?>

<!--
    docbook2latex.xslt:
        translates a DocBook XML source into a LaTeX source file,
        which can be processed with pdflatex to produce a
        pretty PDF file.

        Note: In the LaTeX output, this XSLT encodes all quotes
        with \QUOTE{} commands, which are not defined in this
        file. This is because XSLT does not support regular
        expressions natively and therefore it is rather difficult
        to implement proper "pretty quotes" (different glyphs for
        opening and closing quotes) in XSLT. The doc/manual/
        makefile solves this by running sed over the LaTeX source
        once more, replacing all \QUOTE{} commands with
        \OQ{} and \CQ{} commands, which _are_ defined to the
        pretty quotes for English in the LaTeX output generated
        by this XSLT (see below).
-->
<!--
    Copyright (C) 2006-2023 Oracle and/or its affiliates.

    This file is part of VirtualBox base platform packages, as
    available from https://www.virtualbox.org.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation, in version 3 of the
    License.

    This program is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, see <https://www.gnu.org/licenses>.

    SPDX-License-Identifier: GPL-3.0-only
-->

<xsl:stylesheet
  version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:xsd="http://www.w3.org/2001/XMLSchema"
  xmlns:str="http://xsltsl.org/string"
>

  <xsl:import href="string.xsl"/>
  <xsl:import href="common-formatcfg.xsl"/>

  <xsl:variable name="g_nlsChapter">
    <xsl:choose>
      <xsl:when test="$TARGETLANG='de_DE'">Kapitel</xsl:when>
      <xsl:when test="$TARGETLANG='fr_FR'">chapitre</xsl:when>
      <xsl:when test="$TARGETLANG='en_US'">chapter</xsl:when>
      <xsl:otherwise>
        <xsl:message terminate="yes"><xsl:value-of select="concat('Invalid language ', $TARGETLANG)" /></xsl:message>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:variable>

  <xsl:variable name="g_nlsPage">
    <xsl:choose>
      <xsl:when test="$TARGETLANG='de_DE'">auf Seite</xsl:when>
      <xsl:when test="$TARGETLANG='fr_FR'">page</xsl:when>
      <xsl:when test="$TARGETLANG='en_US'">page</xsl:when>
      <xsl:otherwise>
        <xsl:message terminate="yes"><xsl:value-of select="concat('Invalid language ', $TARGETLANG)" /></xsl:message>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:variable>

  <xsl:variable name="g_nlsNote">
    <xsl:choose>
      <xsl:when test="$TARGETLANG='de_DE'">Hinweis</xsl:when>
      <xsl:when test="$TARGETLANG='fr_FR'">Note</xsl:when>
      <xsl:when test="$TARGETLANG='en_US'">Note</xsl:when>
      <xsl:otherwise>
        <xsl:message terminate="yes"><xsl:value-of select="concat('Invalid language ', $TARGETLANG)" /></xsl:message>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:variable>

    <xsl:variable name="g_nlsWarning">
    <xsl:choose>
      <xsl:when test="$TARGETLANG='de_DE'">Warnung</xsl:when>
      <xsl:when test="$TARGETLANG='fr_FR'">Avertissement</xsl:when>
      <xsl:when test="$TARGETLANG='en_US'">Warning</xsl:when>
      <xsl:otherwise>
        <xsl:message terminate="yes"><xsl:value-of select="concat('Invalid language ', $TARGETLANG)" /></xsl:message>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:variable>

  <!-- command synopsis -->
  <xsl:variable name="arg.rep.repeat.str.tex">\ldots{}</xsl:variable>
  <xsl:variable name="arg.or.sep.tex"> |~</xsl:variable>

  <xsl:output method="text"/>

  <xsl:strip-space elements="*"/>
  <xsl:preserve-space elements="para"/>

  <xsl:template match="/book">
    <xsl:text>
\documentclass[oneside,a4paper,10pt,DIV10]{scrbook}
\usepackage{geometry}
\geometry{top=3cm,bottom=4cm}
\usepackage[T1]{fontenc}
\usepackage{tabulary}
\usepackage[pdftex,
            a4paper,
            colorlinks=true,
            linkcolor=blue,
            urlcolor=darkgreen,
            bookmarksnumbered,
            bookmarksopen=true,
            bookmarksopenlevel=0,
            hyperfootnotes=false,
            plainpages=false,
            pdfpagelabels
  ]{hyperref}

\usepackage{nameref}
\usepackage{graphicx}
\usepackage{hyperref}
\usepackage{fancybox}
\usepackage{alltt}
\usepackage{color}
\usepackage{scrextend}
\definecolor{darkgreen}{rgb}{0,0.6,0}
\tymin=21pt

</xsl:text>
  <xsl:if test="$TARGETLANG='de_DE'">\usepackage[ngerman]{babel}&#10;\PrerenderUnicode{ü}</xsl:if>
<!--   <xsl:if test="$TARGETLANG='fr_FR'">\usepackage[french]{babel}&#10;\FrenchItemizeSpacingfalse&#10;\renewcommand{\FrenchLabelItem}{\textbullet}</xsl:if>
    this command is no longer understood by TexLive2008
    -->
    <xsl:text>

% use Palatino as serif font:
% \usepackage{mathpazo}
\usepackage{charter}
% use Helvetica as sans-serif font:
\usepackage{helvet}

% use Bera Mono (a variant of Bitstream Vera Mono) as typewriter font
% (requires texlive-fontsextra)
\usepackage[scaled]{beramono}
% previously: use Courier as typewriter font:
% \usepackage{courier}

\definecolor{colNote}{rgb}{0,0,0}
\definecolor{colWarning}{rgb}{0,0,0}
\definecolor{colScreenFrame}{rgb}{0,0,0}
\definecolor{colScreenText}{rgb}{0,0,0}

% number headings down to this level
\setcounter{secnumdepth}{3}
% more space for the section numbers
\makeatletter
\renewcommand*\l@section{\@dottedtocline{1}{1.5em}{2.9em}}
\renewcommand*\l@subsection{\@dottedtocline{2}{4.4em}{3.8em}}
\renewcommand*\l@subsubsection{\@dottedtocline{3}{8.2em}{3.8em}}
\renewcommand*\@pnumwidth{1.7em}
\renewcommand*\@tocrmarg{5.0em}
\makeatother

% more tolerance at 2nd wrap stage:
\tolerance = 1000
% allow 3rd wrap stage:
\emergencystretch = 10pt
% no Schusterjungen:
\clubpenalty = 10000
% no Hurenkinder:
\widowpenalty = 10000
\displaywidowpenalty = 10000
% max pdf compression:
\pdfcompresslevel9

% opening and closing quotes: the OQ and CQ macros define this (and the makefile employs some sed magic also)
</xsl:text>
  <xsl:choose>
    <xsl:when test="$TARGETLANG='de_DE'">
      <xsl:text>\newcommand\OQ{\texorpdfstring{\glqq}{"}}&#10;\newcommand\CQ{\texorpdfstring{\grqq}{"}}&#10;</xsl:text>
    </xsl:when>
    <xsl:when test="$TARGETLANG='fr_FR'">
      <xsl:text>\newcommand\OQ{\texorpdfstring{``}{"}}&#10;\newcommand\CQ{\texorpdfstring{''}{"}}&#10;</xsl:text>
    </xsl:when>
    <xsl:when test="$TARGETLANG='en_US'">
      <xsl:text>\newcommand\OQ{\texorpdfstring{``}{"}}&#10;\newcommand\CQ{\texorpdfstring{''}{"}}&#10;</xsl:text>
    </xsl:when>
    <xsl:otherwise>
      <xsl:message terminate="yes"><xsl:value-of select="concat('Invalid language ', $TARGETLANG)" /></xsl:message>
    </xsl:otherwise>
  </xsl:choose>

  <xsl:apply-templates />

  <xsl:text>
\end{sloppypar}
\end{document}
  </xsl:text>

  </xsl:template>

  <xsl:template match="bookinfo">
    <xsl:apply-templates />
    <xsl:text>&#x0a;\newcommand\docbookbookinfocopyright{Copyright \copyright{} \docbookbookinfocopyrightyear{} \docbookbookinfocopyrightholder{}}&#x0a;
\author{ \docbooktitleedition \\ %
\\ %
</xsl:text>
    <xsl:if test="//bookinfo/address">
      <xsl:text>\docbookbookinfoaddress \\ %
\\ %
</xsl:text>
    </xsl:if>
    <xsl:text>\docbookbookinfocopyright \\ %
}

\title{\docbooktitle \\
\docbooksubtitle}
% \subtitle{\docbooksubtitle}
\hypersetup{pdfauthor=\docbookcorpauthor}
\hypersetup{pdftitle=\docbooktitle{} \docbooksubtitle{}}

\hyphenation{da-ta-ba-ses}
\hyphenation{deb-conf}
\hyphenation{VirtualBox}

\begin{document}
\frontmatter
% bird/2018-05-14: Use sloppypar so we don't push path names and other long words
%                  thru the right margin.  TODO: Find better solution? microtype?
\begin{sloppypar}
% \maketitle
%\begin{titlepage}
\thispagestyle{empty}
\begin{minipage}{\textwidth}
\begin{center}
\includegraphics[width=4cm]{images/vboxlogo.png}
\end{center}%
\vspace{10mm}

{\fontsize{40pt}{40pt}\selectfont\rmfamily\bfseries%
\begin{center}
\docbooktitle
\end{center}%
\vspace{10mm}
}

{\fontsize{30pt}{30pt}\selectfont\rmfamily\bfseries%
\begin{center}
\docbooksubtitle
\end{center}%
\vspace{10mm}
}

{\fontsize{16pt}{20pt}\selectfont\rmfamily%
\begin{center}
</xsl:text>
    <xsl:if test="//bookinfo/othercredit">
      <xsl:text>\docbookbookinfoothercreditcontrib{}: \docbookbookinfoothercreditfirstname{} \docbookbookinfoothercreditsurname

\vspace{8mm}
</xsl:text>
    </xsl:if>
    <xsl:text>\docbooktitleedition

\vspace{2mm}

\docbookbookinfocopyright

\vspace{2mm}

\docbookbookinfoaddress
\end{center}%
}

%\end{titlepage}
\end{minipage}

\tableofcontents
  </xsl:text>
  </xsl:template>

  <xsl:template match="subtitle">
    <xsl:choose>
      <xsl:when test="name(..)='bookinfo'">
        <xsl:text>\newcommand\docbooksubtitle{</xsl:text>
        <xsl:apply-templates />
        <xsl:text>}</xsl:text>
      </xsl:when>
    </xsl:choose>
  </xsl:template>

  <!-- Determins the section depth, returning a number 1,2,3,4,5,6,7,... -->
  <xsl:template name="get-section-level">
    <xsl:param name="a_Node" select=".."/>
    <xsl:for-each select="$a_Node"> <!-- makes it current -->
      <xsl:choose>
        <xsl:when test="self::sect1"><xsl:text>1</xsl:text></xsl:when>
        <xsl:when test="self::sect2"><xsl:text>2</xsl:text></xsl:when>
        <xsl:when test="self::sect3"><xsl:text>3</xsl:text></xsl:when>
        <xsl:when test="self::sect4"><xsl:text>4</xsl:text></xsl:when>
        <xsl:when test="self::sect5"><xsl:text>5</xsl:text></xsl:when>
        <xsl:when test="self::section">
          <xsl:value-of select="count(ancestor::section) + 1"/>
        </xsl:when>
        <xsl:when test="self::simplesect">
          <xsl:variable name="tmp">
            <xsl:call-template name="get-section-level">
              <xsl:with-param name="a_Node" select="parent::*"/>
            </xsl:call-template>
          </xsl:variable>
          <xsl:value-of select="$tmp + 1"/>
        </xsl:when>
        <xsl:when test="self::preface"><xsl:text>0</xsl:text></xsl:when>
        <xsl:when test="self::chapter"><xsl:text>0</xsl:text></xsl:when>
        <xsl:when test="self::appendix"><xsl:text>0</xsl:text></xsl:when>
        <xsl:when test="self::article"><xsl:text>0</xsl:text></xsl:when>
        <xsl:otherwise>
          <xsl:message terminate="yes">get-section-level was called on non-section element: <xsl:value-of select="."/> </xsl:message>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:for-each>
  </xsl:template>

  <!--
    Inserts \hypertarget{@id} that can be referenced via the /A "nameddest=@id"
    command line or #nameddest=@id URL parameter.

    TODO: The placement of the target could be improved on. The raisebox
          stuff is a crude hack to make it a little more acceptable.  -->
  <xsl:template name="title-wrapper">
    <xsl:param name="texcmd" select="concat('\',name(..))"/>
    <xsl:param name="refid" select="../@id"/>
    <xsl:param name="role" select="../@role"/>

    <xsl:call-template name="xsltprocNewlineOutputHack"/>
    <xsl:if test="$texcmd='\chapter' and (name(../preceding-sibling::*[1])='preface' or name(../preceding-sibling::*[1])='bookinfo')">
      <xsl:text>\mainmatter&#x0a;</xsl:text>
    </xsl:if>
    <xsl:choose>
      <xsl:when test="$refid">
        <xsl:text>&#x0a;</xsl:text>
        <xsl:value-of select="$texcmd"/>
        <xsl:if test="not(contains($texcmd, '*'))">
          <xsl:text>[</xsl:text> <!-- for toc -->
          <xsl:apply-templates />
          <xsl:text>]</xsl:text>
        </xsl:if>
        <xsl:text>{</xsl:text> <!-- for doc -->
        <xsl:text>\raisebox{\ht\strutbox}{\hypertarget{</xsl:text>
        <xsl:value-of select="$refid"/>
        <xsl:text>}{}}</xsl:text>
        <xsl:apply-templates />
        <xsl:text>}</xsl:text>
      </xsl:when>
      <xsl:otherwise>
        <xsl:text>&#x0a;</xsl:text><xsl:value-of select="$texcmd"/><xsl:text>{</xsl:text>
        <xsl:apply-templates />
        <xsl:text>}</xsl:text>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template match="title">
    <xsl:variable name="refid" select="../@id" />
    <xsl:choose>
      <xsl:when test="name(..)='bookinfo'">
        <xsl:text>\newcommand\docbooktitle{</xsl:text>
        <xsl:apply-templates />
        <xsl:text>}</xsl:text>
      </xsl:when>
      <xsl:when test="name(..)='chapter'">
        <xsl:call-template name="title-wrapper"/>
      </xsl:when>
      <xsl:when test="name(..)='preface'">
        <xsl:call-template name="title-wrapper">
          <xsl:with-param name="texcmd">\chapter</xsl:with-param>
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="name(..)='sect1'">
        <xsl:call-template name="title-wrapper">
          <xsl:with-param name="texcmd">\section</xsl:with-param>
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="parent::sect2[@role='not-in-toc'] or parent::refsect1 or (parent::section and count(ancestor::section) = 2)">
        <xsl:call-template name="title-wrapper">
          <xsl:with-param name="texcmd">\subsection*</xsl:with-param>
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="name(..)='sect2'">
        <xsl:call-template name="title-wrapper">
          <xsl:with-param name="texcmd">\subsection</xsl:with-param>
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="parent::sect3[@role='not-in-toc'] or parent::refsect2 or (parent::section and count(ancestor::section) = 3)">
        <xsl:call-template name="title-wrapper">
          <xsl:with-param name="texcmd">\subsubsection*</xsl:with-param>
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="name(..)='sect3'">
        <xsl:call-template name="title-wrapper">
          <xsl:with-param name="texcmd">\subsubsection</xsl:with-param>
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="parent::sect4[@role='not-in-toc'] or parent::refsect3 or (parent::section and count(ancestor::section) = 4)">
        <xsl:call-template name="title-wrapper">
          <xsl:with-param name="texcmd">\paragraph*</xsl:with-param>
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="name(..)='sect4'">
        <xsl:call-template name="title-wrapper">
          <xsl:with-param name="texcmd">\paragraph</xsl:with-param>
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="parent::sect5[@role='not-in-toc'] or parent::refsect4 or (parent::section and count(ancestor::section) = 5)">
        <xsl:call-template name="title-wrapper">
          <xsl:with-param name="texcmd">\subparagraph*</xsl:with-param>
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="name(..)='sect5'">
        <xsl:call-template name="title-wrapper">
          <xsl:with-param name="texcmd">\subparagraph</xsl:with-param>
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="name(..)='appendix'">
        <xsl:call-template name="title-wrapper">
          <xsl:with-param name="texcmd">\chapter</xsl:with-param>
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="name(..)='glossdiv'">
        <xsl:call-template name="title-wrapper">
          <xsl:with-param name="texcmd">\section*</xsl:with-param>
        </xsl:call-template>
      </xsl:when>

      <xsl:when test="parent::simplesect">
        <xsl:if test="../@role">
          <xsl:message terminate="yes">Role not allowed with simplesect: <xsl:value-of select="../@role"/></xsl:message>
        </xsl:if>
        <xsl:variable name="level">
          <xsl:call-template name="get-section-level">
            <xsl:with-param name="a_Node" select=".."/>
          </xsl:call-template>
        </xsl:variable>
        <xsl:choose>
          <xsl:when test="$level = 1">
            <xsl:call-template name="title-wrapper"><xsl:with-param name="texcmd">\section*</xsl:with-param></xsl:call-template>
          </xsl:when>
          <xsl:when test="$level = 2">
            <xsl:call-template name="title-wrapper"><xsl:with-param name="texcmd">\subsection*</xsl:with-param></xsl:call-template>
          </xsl:when>
          <xsl:when test="$level = 3">
            <xsl:call-template name="title-wrapper"><xsl:with-param name="texcmd">\subsubsection*</xsl:with-param></xsl:call-template>
          </xsl:when>
          <xsl:when test="$level = 4">
            <xsl:call-template name="title-wrapper"><xsl:with-param name="texcmd">\paragraph*</xsl:with-param></xsl:call-template>
          </xsl:when>
          <xsl:when test="$level = 5">
            <xsl:call-template name="title-wrapper"><xsl:with-param name="texcmd">\subparagraph*</xsl:with-param></xsl:call-template>
          </xsl:when>
          <xsl:otherwise>
            <xsl:message terminate="yes">Unsupported simplesect/title depth: <xsl:value-of select="$level"/></xsl:message>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:when>

    </xsl:choose>
    <xsl:if test="$refid">
      <xsl:value-of select="concat('&#x0a;\label{', $refid, '}')" />
    </xsl:if>
    <xsl:text>&#x0a;</xsl:text>
  </xsl:template>

  <xsl:template match="edition">
    <xsl:choose>
      <xsl:when test="name(..)='bookinfo'">
        <xsl:text>\newcommand\docbooktitleedition{</xsl:text>
        <xsl:apply-templates />
        <xsl:text>}&#x0a;</xsl:text>
      </xsl:when>
    </xsl:choose>
  </xsl:template>

  <xsl:template match="corpauthor">
    <xsl:choose>
      <xsl:when test="name(..)='bookinfo'">
        <xsl:text>\newcommand\docbookcorpauthor{</xsl:text>
        <xsl:apply-templates />
        <xsl:text>}&#x0a;</xsl:text>
      </xsl:when>
    </xsl:choose>
  </xsl:template>

  <xsl:template match="address">
    <xsl:choose>
      <xsl:when test="name(..)='bookinfo'">
        <xsl:text>\newcommand\docbookbookinfoaddress{</xsl:text>
        <xsl:apply-templates />
        <xsl:text>}&#x0a;</xsl:text>
      </xsl:when>
    </xsl:choose>
  </xsl:template>

  <xsl:template match="year">
    <xsl:choose>
      <xsl:when test="name(..)='copyright'">
        <xsl:text>\newcommand\docbookbookinfocopyrightyear{</xsl:text>
        <xsl:apply-templates />
        <xsl:text>}&#x0a;</xsl:text>
      </xsl:when>
    </xsl:choose>
  </xsl:template>

  <xsl:template match="holder">
    <xsl:choose>
      <xsl:when test="name(..)='copyright'">
        <xsl:text>\newcommand\docbookbookinfocopyrightholder{</xsl:text>
        <xsl:apply-templates />
        <xsl:text>}&#x0a;</xsl:text>
      </xsl:when>
    </xsl:choose>
  </xsl:template>

  <xsl:template match="firstname">
    <xsl:choose>
      <xsl:when test="name(..)='othercredit'">
        <xsl:text>\newcommand\docbookbookinfoothercreditfirstname{</xsl:text>
        <xsl:apply-templates />
        <xsl:text>}&#x0a;</xsl:text>
      </xsl:when>
    </xsl:choose>
  </xsl:template>

  <xsl:template match="surname">
    <xsl:choose>
      <xsl:when test="name(..)='othercredit'">
        <xsl:text>\newcommand\docbookbookinfoothercreditsurname{</xsl:text>
        <xsl:apply-templates />
        <xsl:text>}&#x0a;</xsl:text>
      </xsl:when>
    </xsl:choose>
  </xsl:template>

  <xsl:template match="contrib">
    <xsl:choose>
      <xsl:when test="name(..)='othercredit'">
        <xsl:text>\newcommand\docbookbookinfoothercreditcontrib{</xsl:text>
        <xsl:apply-templates />
        <xsl:text>}&#x0a;</xsl:text>
      </xsl:when>
    </xsl:choose>
  </xsl:template>

  <xsl:template match="glossary">
    <xsl:text>&#x0a;&#x0a;\backmatter&#x0a;\chapter{Glossary}&#x0a;</xsl:text>
    <xsl:apply-templates />
  </xsl:template>

  <xsl:template match="para">
    <xsl:if test="not(name(..)='footnote' or name(..)='note' or name(..)='warning' or name(..)='entry' or (name(../..)='varlistentry' and position()=1))">
      <xsl:text>&#x0a;&#x0a;</xsl:text>
    </xsl:if>
    <xsl:apply-templates />
  </xsl:template>

  <xsl:template match="note">
    <xsl:value-of select="concat('&#x0a;&#x0a;\vspace{.2cm}&#x0a;&#x0a;\begin{center}\fbox{\begin{minipage}[c]{0.9\textwidth}\color{colNote}\textbf{', $g_nlsNote, ':} ')" />
    <xsl:apply-templates />
    <xsl:text>\end{minipage}}\end{center}&#x0a;&#x0a;\vspace{.2cm}&#x0a;&#x0a;</xsl:text>
  </xsl:template>

  <xsl:template match="warning">
    <xsl:value-of select="concat('&#x0a;&#x0a;\vspace{.2cm}&#x0a;&#x0a;\begin{center}\fbox{\begin{minipage}[c]{0.9\textwidth}\color{colWarning}\textbf{', $g_nlsWarning, ':} ')" />
    <xsl:apply-templates />
    <xsl:text>\end{minipage}}\end{center}&#x0a;&#x0a;\vspace{.2cm}&#x0a;&#x0a;</xsl:text>
  </xsl:template>

  <xsl:template match="screen">
    <xsl:text>&#x0a;&#x0a;{\footnotesize\begin{alltt}&#x0a;</xsl:text>
    <xsl:apply-templates />
    <xsl:text>&#x0a;\end{alltt}}&#x0a;</xsl:text>
  </xsl:template>

  <xsl:template match="programlisting">
    <xsl:text>&#x0a;&#x0a;{\small\begin{alltt}&#x0a;</xsl:text>
    <xsl:apply-templates />
    <xsl:text>&#x0a;\end{alltt}}&#x0a;</xsl:text>
  </xsl:template>

  <xsl:template match="footnote">
    <xsl:text>\footnote{</xsl:text>
    <xsl:apply-templates />
    <xsl:text>}</xsl:text>
  </xsl:template>

  <xsl:template match="tgroup">
    <xsl:choose>
      <xsl:when test="@style='verywide'">
        <xsl:text>&#x0a;&#x0a;{\small\begin{center}&#x0a;\begin{tabulary}{1.1\textwidth}[]</xsl:text>
      </xsl:when>
      <xsl:otherwise>
        <xsl:text>&#x0a;&#x0a;{\small\begin{center}&#x0a;\begin{tabulary}{.9\textwidth}[]</xsl:text>
      </xsl:otherwise>
    </xsl:choose>
    <xsl:text>{</xsl:text>
    <xsl:choose>
      <xsl:when test="@cols='1'">
        <xsl:text>|L|</xsl:text>
      </xsl:when>
      <xsl:when test="@cols='2'">
        <xsl:text>|L|L|</xsl:text>
      </xsl:when>
      <xsl:when test="@cols='3'">
        <xsl:text>|L|L|L|</xsl:text>
      </xsl:when>
      <xsl:when test="@cols='4'">
        <xsl:text>|L|L|L|L|</xsl:text>
      </xsl:when>
      <xsl:when test="@cols='5'">
        <xsl:text>|L|L|L|L|L|</xsl:text>
      </xsl:when>
      <xsl:when test="@cols='6'">
        <xsl:text>|L|L|L|L|L|L|</xsl:text>
      </xsl:when>
      <xsl:otherwise>
        <xsl:message terminate="yes">Unsupported number of columns (<xsl:value-of select="@cols"/>), fix document or converter</xsl:message>
      </xsl:otherwise>
    </xsl:choose>
    <xsl:text>}&#x0a;\hline&#x0a;</xsl:text>
    <xsl:apply-templates />
    <xsl:text>&#x0a;\end{tabulary}&#x0a;\end{center}}&#x0a;</xsl:text>
  </xsl:template>

  <xsl:template match="row">
    <xsl:apply-templates />
    <xsl:text>&#x0a;\\ \hline&#x0a;</xsl:text>
  </xsl:template>

  <xsl:template match="entry">
    <xsl:if test="not(position()=1)">
      <xsl:text> &amp; </xsl:text>
    </xsl:if>
    <xsl:apply-templates />
  </xsl:template>

  <xsl:template match="itemizedlist">
    <xsl:call-template name="xsltprocNewlineOutputHack"/>
    <xsl:text>&#x0a;\begin{itemize}&#x0a;</xsl:text>
    <xsl:if test="@spacing = 'compact'">
      <xsl:text> \setlength{\parskip}{0pt}&#x0a;</xsl:text>
      <xsl:text> \setlength{\itemsep}{0pt}&#x0a;</xsl:text>
      <xsl:text> \setlength{\topsep}{0pt}&#x0a;</xsl:text>
      <xsl:text> \setlength{\parsep}{0pt}&#x0a;</xsl:text>
      <xsl:text> \setlength{\partopsep}{0pt}&#x0a;</xsl:text>
    </xsl:if>
    <xsl:apply-templates />
    <xsl:text>&#x0a;\end{itemize}&#x0a;</xsl:text>
  </xsl:template>

  <xsl:template match="orderedlist">
    <xsl:call-template name="xsltprocNewlineOutputHack"/>
    <xsl:text>&#x0a;\begin{enumerate}&#x0a;</xsl:text>
    <xsl:if test="@spacing = 'compact'">
      <xsl:text> \setlength{\parskip}{0pt}&#x0a;</xsl:text>
      <xsl:text> \setlength{\itemsep}{0pt}&#x0a;</xsl:text>
      <xsl:text> \setlength{\topsep}{0pt}&#x0a;</xsl:text>
      <xsl:text> \setlength{\parsep}{0pt}&#x0a;</xsl:text>
      <xsl:text> \setlength{\partopsep}{0pt}&#x0a;</xsl:text>
    </xsl:if>
    <xsl:apply-templates />
    <xsl:text>&#x0a;\end{enumerate}&#x0a;</xsl:text>
  </xsl:template>

  <xsl:template match="variablelist">
    <xsl:call-template name="xsltprocNewlineOutputHack"/>
    <xsl:text>&#x0a;\begin{description}&#x0a;</xsl:text>
    <xsl:if test="@spacing = 'compact'">
      <xsl:text> \setlength{\parskip}{0pt}&#x0a;</xsl:text>
      <xsl:text> \setlength{\itemsep}{0pt}&#x0a;</xsl:text>
      <xsl:text> \setlength{\topsep}{0pt}&#x0a;</xsl:text>
      <xsl:text> \setlength{\parsep}{0pt}&#x0a;</xsl:text>
      <xsl:text> \setlength{\partopsep}{0pt}&#x0a;</xsl:text>
    </xsl:if>
    <xsl:apply-templates />
    <xsl:text>&#x0a;\end{description}&#x0a;</xsl:text>
  </xsl:template>

  <xsl:template match="varlistentry">
    <xsl:if test="not(./term) or not(./listitem) or count(./listitem) != 1">
      <xsl:message terminate="yes">Expected at least one term and one listitem element in the varlistentry.</xsl:message>
    </xsl:if>
    <xsl:text>&#x0a;&#x0a;\item[{\parbox[t]{\linewidth}{\raggedright </xsl:text>
    <xsl:apply-templates select="./term[1]"/>
    <xsl:for-each select="./term[position() > 1]">
      <xsl:text>\\&#x0a; </xsl:text>
      <xsl:apply-templates select="."/>
    </xsl:for-each>
    <xsl:text>}}]\hfill\\</xsl:text>
    <xsl:apply-templates select="listitem/*"/>
  </xsl:template>

  <xsl:template match="listitem">
    <xsl:text>&#x0a;&#x0a;\item </xsl:text>
    <xsl:apply-templates />
    <xsl:text>&#x0a;</xsl:text>
  </xsl:template>

  <xsl:template match="glossterm">
    <xsl:variable name="refid" select="(@id)" />
    <xsl:if test="$refid">
      <xsl:value-of select="concat('&#x0a;\label{', $refid, '}')" />
    </xsl:if>
    <xsl:text>&#x0a;&#x0a;\item[</xsl:text>
    <xsl:apply-templates />
    <xsl:text>]</xsl:text>
  </xsl:template>

  <xsl:template match="glosslist | glossdiv">
    <xsl:text>&#x0a;&#x0a;\begin{description}&#x0a;</xsl:text>
    <xsl:apply-templates />
    <xsl:text>&#x0a;\end{description}&#x0a;</xsl:text>
  </xsl:template>

  <xsl:template match="superscript">
    <xsl:variable name="contents">
      <xsl:apply-templates />
    </xsl:variable>
    <xsl:value-of select="concat('\texorpdfstring{\textsuperscript{', $contents, '}}{', $contents, '}')" />
  </xsl:template>

  <xsl:template match="emphasis">
    <xsl:choose>
      <xsl:when test="@role='bold'">
        <xsl:text>\textbf{</xsl:text>
      </xsl:when>
      <xsl:otherwise>
        <xsl:text>\textit{</xsl:text>
      </xsl:otherwise>
    </xsl:choose>
    <xsl:apply-templates />
    <xsl:text>}</xsl:text>
  </xsl:template>

  <xsl:template match="computeroutput | code">
    <xsl:text>\texttt{</xsl:text>
    <xsl:apply-templates />
    <xsl:text>}</xsl:text>
  </xsl:template>

  <xsl:template match="literal | filename">
    <xsl:text>\texttt{</xsl:text>
    <xsl:apply-templates />
    <xsl:text>}</xsl:text>
  </xsl:template>

  <xsl:template match="citetitle">
    <xsl:text>\textit{</xsl:text>
    <xsl:apply-templates />
    <xsl:text>}</xsl:text>
  </xsl:template>

  <xsl:template match="lineannotation">
    <xsl:text>\textit{</xsl:text>
    <xsl:apply-templates />
    <xsl:text>}</xsl:text>
  </xsl:template>

  <xsl:template match="ulink[@url!='' and not(text())]">
    <xsl:text>\url{</xsl:text>
    <xsl:value-of select="@url"/>
    <xsl:text>}</xsl:text>
  </xsl:template>

  <xsl:template match="ulink[@url!='' and text()]">
    <xsl:text>\href{</xsl:text>
    <xsl:value-of select="@url"/>
    <xsl:text>}{</xsl:text>
    <xsl:apply-templates />
    <xsl:text>}</xsl:text>
  </xsl:template>

  <xsl:template match="ulink[(@url='' or not(@url)) and text()]">
    <xsl:text>\url{</xsl:text>
    <xsl:apply-templates />
    <xsl:text>}</xsl:text>
  </xsl:template>

  <xsl:template match="xref">
    <xsl:choose>
      <xsl:when test="@endterm">
        <xsl:value-of select="concat('\hyperref[', @linkend, ']{\mbox{', @endterm, '}}')" />
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="concat($g_nlsChapter, ' \ref{', @linkend, '}, \textit{\nameref{', @linkend, '}}, ', $g_nlsPage, ' \pageref{', @linkend, '}')" />
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template match="link">
    <xsl:choose>
      <xsl:when test="@endterm">
        <xsl:value-of select="concat('\hyperref[', @linkend, ']{\mbox{', @endterm, '}}')" />
      </xsl:when>
      <xsl:when test="./text()">
        <xsl:value-of select="concat('\hyperref[', @linkend, ']{\mbox{')" />
        <xsl:apply-templates select="./text()"/>
        <xsl:value-of select="'}}'" />
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="concat($g_nlsChapter, ' \ref{', @linkend, '}, \textit{\nameref{', @linkend, '}}, ', $g_nlsPage, ' \pageref{', @linkend, '}')" />
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template match="trademark">
    <xsl:apply-templates />
    <xsl:text>\textsuperscript{\textregistered}</xsl:text>
  </xsl:template>

  <!-- for some reason, DocBook insists of having image data nested this way always:
       mediaobject -> imageobject -> imagedata
       but only imagedata is interesting  -->
  <xsl:template match="imagedata">
    <xsl:if test="@align='center'">
      <xsl:text>\begin{center}</xsl:text>
    </xsl:if>
    <xsl:value-of select="concat('&#x0a;\includegraphics[width=', @width, ']{', @fileref, '}&#x0a;')" />
    <xsl:apply-templates />
    <xsl:if test="@align='center'">
      <xsl:text>\end{center}</xsl:text>
    </xsl:if>
  </xsl:template>

  <!--
     Turn the refsynopsisdiv part of a manpage into a named & indented paragraph.
  -->
  <xsl:template match="refsynopsisdiv">
    <xsl:if test="name(*[1]) != 'cmdsynopsis'"><xsl:message terminate="yes">Expected refsynopsisdiv to start with cmdsynopsis</xsl:message></xsl:if>
    <xsl:if test="title"><xsl:message terminate="yes">No title element supported in refsynopsisdiv</xsl:message></xsl:if>
    <xsl:call-template name="xsltprocNewlineOutputHack"/>
    <xsl:text>&#x0a;\subsection*{Synopsis}&#x0a;</xsl:text>
    <xsl:apply-templates />
  </xsl:template>

  <!--
    The refsect1 is used for 'Description' and such. Do same as with refsynopsisdiv
    and turn it into a named & indented paragraph.
    -->
  <xsl:template match="refsect1">
    <xsl:if test="name(*[1]) != 'title' or count(title) != 1">
      <xsl:message terminate="yes">Expected exactly one title as the first refsect1 element (remarks goes after title!).</xsl:message>
    </xsl:if>
    <xsl:apply-templates/>
  </xsl:template>

  <!--
    The refsect2 element will be turned into a subparagraph if it has a title,
    however, that didn't work out when it didn't have a title and started with
    a cmdsynopsis instead (subcommand docs).  So, we're doing some trickery
    here (HACK ALERT) for the non-title case to feign a paragraph.
    -->
  <xsl:template match="refsect2">
    <xsl:if test="name(*[1]) != 'title' or count(title) != 1">
      <xsl:message terminate="yes">Expected exactly one title as the first refsect2 element (remarks goes after title!).</xsl:message>
    </xsl:if>
    <xsl:apply-templates/>
    <xsl:text>&#x0a;</xsl:text>
  </xsl:template>


  <!--
    Command Synopsis elements.

    We treat each command element inside a cmdsynopsis as the start of
    a new paragraph.  The DocBook HTML converter does so too, but the
    manpage one doesn't.

    sbr and linebreaks made by latex should be indented from the base
    command level. This is done by the \hangindent3em\hangafter1 bits.

    We exploit the default paragraph indentation to get each command
    indented from the left margin.  This, unfortunately, doesn't work
    if we're the first paragraph in a (sub*)section.  \noindent cannot
    counter this due to when latex enforces first paragraph stuff. Since
    it's tedious to figure out when we're in the first paragraph and when
    not, we just do \noindent\hspace{1em} everywhere.
    -->
  <xsl:template match="sbr">
    <xsl:if test="not(ancestor::cmdsynopsis)">
      <xsl:message terminate="yes">sbr only supported inside cmdsynopsis (because of hangindent)</xsl:message>
    </xsl:if>
    <xsl:text>\newline</xsl:text>
  </xsl:template>

  <xsl:template match="refentry|refnamediv|refentryinfo|refmeta|refsect3|refsect4|refsect5|synopfragment|synopfragmentref|cmdsynopsis/info">
    <xsl:message terminate="yes"><xsl:value-of select="name()"/> is not supported</xsl:message>
  </xsl:template>

  <xsl:template match="cmdsynopsis">
    <xsl:if test="preceding-sibling::cmdsynopsis">
      <xsl:text>%cmdsynopsis</xsl:text>
    </xsl:if>
    <xsl:text>&#x0a;</xsl:text>
    <xsl:text>\begin{flushleft}</xsl:text>
    <xsl:if test="parent::remark[@role='VBoxManage-overview']">
      <!-- Overview fontsize trick -->
      <xsl:text>{\footnotesize</xsl:text>
    </xsl:if>
    <xsl:text>\noindent\hspace{1em}</xsl:text>
    <xsl:text>\hangindent3em\hangafter1\texttt{</xsl:text>
    <xsl:apply-templates />
    <xsl:text>}</xsl:text>
    <xsl:if test="following-sibling::*">
    </xsl:if>

    <!-- For refsect2 subcommand descriptions. -->
    <xsl:if test="not(following-sibling::cmdsynopsis) and position() != last()">
      <xsl:text>\linebreak</xsl:text>
    </xsl:if>
    <!-- Special overview trick for the current VBoxManage command overview. -->
    <xsl:if test="parent::remark[@role='VBoxManage-overview']">
      <xsl:text>\par}</xsl:text>
    </xsl:if>
    <xsl:text>\end{flushleft}</xsl:text>
  </xsl:template>

  <xsl:template match="command">
    <xsl:choose>
      <xsl:when test="ancestor::cmdsynopsis">
        <!-- Trigger a line break if this isn't the first command in a synopsis -->
        <xsl:if test="preceding-sibling::command">
          <xsl:text>}\par%command&#x0a;</xsl:text>
          <xsl:text>\noindent\hspace{1em}</xsl:text>
          <xsl:text>\hangindent3em\hangafter1\texttt{</xsl:text>
        </xsl:if>
        <xsl:apply-templates />
      </xsl:when>
      <xsl:otherwise>
        <xsl:text>\texttt{</xsl:text>
        <xsl:apply-templates />
        <xsl:text>}</xsl:text>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template match="option">
    <xsl:choose>
      <xsl:when test="ancestor::cmdsynopsis">
        <xsl:apply-templates />
      </xsl:when>
      <xsl:otherwise>
        <xsl:text>\texttt{</xsl:text>
        <xsl:apply-templates />
        <xsl:text>}</xsl:text>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- duplicated in docbook-refentry-to-C-help.xsl -->
  <xsl:template match="arg|group">
    <!-- separator char if we're not the first child -->
    <xsl:if test="position() > 1">
      <xsl:choose>
        <xsl:when test="parent::group"><xsl:text>\textrm{</xsl:text><xsl:value-of select="$arg.or.sep.tex"/><xsl:text>}</xsl:text></xsl:when>
        <xsl:when test="ancestor-or-self::*/@sepchar"><xsl:value-of select="ancestor-or-self::*/@sepchar"/></xsl:when>
        <xsl:otherwise><xsl:text> </xsl:text></xsl:otherwise>
      </xsl:choose>
    </xsl:if>

    <!-- open wrapping -->
    <xsl:choose>
      <xsl:when test="not(@choice) or @choice = ''">  <xsl:text>\textrm{</xsl:text><xsl:value-of select="$arg.choice.def.open.str"/><xsl:text>}</xsl:text></xsl:when>
      <xsl:when test="@choice = 'opt'">               <xsl:text>\textrm{</xsl:text><xsl:value-of select="$arg.choice.opt.open.str"/><xsl:text>}</xsl:text></xsl:when>
      <xsl:when test="@choice = 'req'">               <xsl:text>\textrm{</xsl:text><xsl:value-of select="$arg.choice.req.open.str"/><xsl:text>}</xsl:text></xsl:when>
      <xsl:when test="@choice = 'plain'"/>
      <xsl:otherwise><xsl:message terminate="yes"><xsl:call-template name="error-prefix"/>Invalid arg choice: "<xsl:value-of select="@choice"/>"</xsl:message></xsl:otherwise>
    </xsl:choose>

    <xsl:apply-templates />

    <!-- repeat indication -->
    <xsl:choose>
      <xsl:when test="@rep = 'norepeat' or not(@rep) or @rep = ''"/>
      <xsl:when test="@rep = 'repeat'">
        <!-- add space padding if we're in a repeating group -->
        <xsl:if test="self::group">
          <xsl:text> </xsl:text>
        </xsl:if>
        <xsl:text>\textrm{</xsl:text><xsl:value-of select="$arg.rep.repeat.str.tex"/><xsl:text>}</xsl:text>
      </xsl:when>
      <xsl:otherwise><xsl:message terminate="yes"><xsl:call-template name="error-prefix"/>Invalid rep choice: "<xsl:value-of select="@rep"/>"</xsl:message></xsl:otherwise>
    </xsl:choose>

    <!-- close wrapping -->
    <xsl:choose>
      <xsl:when test="not(@choice) or @choice = ''">  <xsl:text>\textrm{</xsl:text><xsl:value-of select="$arg.choice.def.close.str"/><xsl:text>}</xsl:text></xsl:when>
      <xsl:when test="@choice = 'opt'">               <xsl:text>\textrm{</xsl:text><xsl:value-of select="$arg.choice.opt.close.str"/><xsl:text>}</xsl:text></xsl:when>
      <xsl:when test="@choice = 'req'">               <xsl:text>\textrm{</xsl:text><xsl:value-of select="$arg.choice.req.close.str"/><xsl:text>}</xsl:text></xsl:when>
    </xsl:choose>

    <!-- add space padding if we're the last element in a nested arg -->
    <xsl:if test="parent::arg and not(following-sibling)">
      <xsl:text> </xsl:text>
    </xsl:if>
  </xsl:template>

  <xsl:template match="replaceable">
    <xsl:choose>
      <xsl:when test="(not(ancestor::cmdsynopsis) and not(ancestor::option) and not(ancestor::screen)) or ancestor::arg">
        <xsl:text>\texttt{\textit{</xsl:text>
        <xsl:apply-templates />
        <xsl:text>}}</xsl:text>
      </xsl:when>
      <xsl:otherwise>
        <xsl:text>\textit{&lt;</xsl:text>
        <xsl:apply-templates />
        <xsl:text>&gt;}</xsl:text>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>


  <!--
    Generic element text magic.
    -->
  <xsl:template match="//text()">

    <!-- Do the translation of \ into \textbackslash{} in two steps, to avoid
         running into replacing {} as well which would be very wrong. -->
    <xsl:variable name="subst1">
      <xsl:call-template name="str:subst">
        <xsl:with-param name="text" select="." />
        <xsl:with-param name="replace" select="'\'" />
        <xsl:with-param name="with" select="'\textbackslash'" />
        <xsl:with-param name="disable-output-escaping" select="no" />
      </xsl:call-template>
    </xsl:variable>
    <xsl:variable name="subst2">
      <xsl:call-template name="str:subst">
        <xsl:with-param name="text" select="$subst1" />
        <xsl:with-param name="replace" select="'{'" />
        <xsl:with-param name="with" select="'\{'" />
        <xsl:with-param name="disable-output-escaping" select="no" />
      </xsl:call-template>
    </xsl:variable>
    <xsl:variable name="subst3">
      <xsl:call-template name="str:subst">
        <xsl:with-param name="text" select="$subst2" />
        <xsl:with-param name="replace" select="'}'" />
        <xsl:with-param name="with" select="'\}'" />
        <xsl:with-param name="disable-output-escaping" select="no" />
      </xsl:call-template>
    </xsl:variable>
    <xsl:variable name="subst4">
      <xsl:call-template name="str:subst">
        <xsl:with-param name="text" select="$subst3" />
        <xsl:with-param name="replace" select="'\textbackslash'" />
        <xsl:with-param name="with" select="'\textbackslash{}'" />
        <xsl:with-param name="disable-output-escaping" select="no" />
      </xsl:call-template>
    </xsl:variable>

    <xsl:choose>
      <xsl:when test="(name(..) = 'computeroutput') or (name(../..) = 'computeroutput')
                   or (name(..) = 'code')           or (name(../..) = 'code')
                   or (name(..) = 'arg')            or (name(../..) = 'arg')
                   or (name(..) = 'option')         or (name(../..) = 'option')
                   or (name(..) = 'command')        or (name(../..) = 'command')
                   or (name(..) = 'cmdsynopsis')    or (name(../..) = 'cmdsynopsis')
                   or (name(..) = 'replaceable')    or (name(../..) = 'replaceable')
                   or (name(..) = 'entry')          or (name(../..) = 'entry')
                     ">
        <xsl:variable name="subst5">
          <xsl:call-template name="str:subst">
            <xsl:with-param name="text" select="translate(normalize-space(concat('&#x7f;',$subst4,'&#x7f;')),'&#x7f;','')" />
            <xsl:with-param name="replace" select="'--'" />
            <xsl:with-param name="with" select="'-{}-'" />
            <xsl:with-param name="disable-output-escaping" select="no" />
          </xsl:call-template>
        </xsl:variable>
        <xsl:variable name="subst6">
          <xsl:call-template name="str:subst">
            <xsl:with-param name="text" select="$subst5" />
            <xsl:with-param name="replace" select="'_'" />
            <xsl:with-param name="with" select="'\_'" />
            <xsl:with-param name="disable-output-escaping" select="no" />
          </xsl:call-template>
        </xsl:variable>
        <xsl:variable name="subst7">
          <xsl:call-template name="str:subst">
            <xsl:with-param name="text" select="$subst6" />
            <xsl:with-param name="replace" select="'$'" />
            <xsl:with-param name="with" select="'\$'" />
            <xsl:with-param name="disable-output-escaping" select="no" />
          </xsl:call-template>
        </xsl:variable>
        <xsl:variable name="subst8">
          <xsl:call-template name="str:subst">
            <xsl:with-param name="text" select="$subst7" />
            <xsl:with-param name="replace" select="'%'" />
            <xsl:with-param name="with" select="'\%'" />
            <xsl:with-param name="disable-output-escaping" select="no" />
          </xsl:call-template>
        </xsl:variable>
        <xsl:variable name="subst9">
          <xsl:call-template name="str:subst">
            <xsl:with-param name="text" select="$subst8" />
            <xsl:with-param name="replace" select="'#'" />
            <xsl:with-param name="with" select="'\#'" />
            <xsl:with-param name="disable-output-escaping" select="no" />
          </xsl:call-template>
        </xsl:variable>
        <xsl:variable name="subst10">
          <xsl:call-template name="str:subst">
            <xsl:with-param name="text" select="$subst9" />
            <xsl:with-param name="replace" select="'~'" />
            <xsl:with-param name="with" select="'\textasciitilde{}'" />
            <xsl:with-param name="disable-output-escaping" select="no" />
          </xsl:call-template>
        </xsl:variable>
        <xsl:variable name="subst11">
          <xsl:call-template name="str:subst">
            <xsl:with-param name="text" select="$subst10" />
            <xsl:with-param name="replace" select="'&amp;'" />
            <xsl:with-param name="with" select="'\&amp;'" />
            <xsl:with-param name="disable-output-escaping" select="no" />
          </xsl:call-template>
        </xsl:variable>
        <xsl:choose>
          <xsl:when test="parent::arg or parent::command">
            <xsl:variable name="subst12">
              <xsl:call-template name="str:subst">
                <xsl:with-param name="text" select="$subst10" />
                <xsl:with-param name="replace" select="' '" />
                <xsl:with-param name="with" select="'~'" />
                <xsl:with-param name="disable-output-escaping" select="no" />
              </xsl:call-template>
            </xsl:variable>
            <xsl:value-of select="$subst12" />
          </xsl:when>
          <xsl:otherwise>
            <xsl:value-of select="$subst11" />
          </xsl:otherwise>
        </xsl:choose>
      </xsl:when>

      <xsl:when test="(name(..)='address') or (name(../..)='address')">
        <xsl:variable name="subst5">
          <xsl:call-template name="str:subst">
            <xsl:with-param name="text" select="$subst4" />
            <xsl:with-param name="replace" select="'&#x0a;'" />
            <xsl:with-param name="with" select="' \\'" />
            <xsl:with-param name="disable-output-escaping" select="no" />
          </xsl:call-template>
        </xsl:variable>
        <xsl:value-of select="$subst5" />
      </xsl:when>

      <!-- <screen> and <programlisting>, which work with alltt environment. -->
      <xsl:otherwise>
        <xsl:variable name="subst5">
          <xsl:call-template name="str:subst">
            <xsl:with-param name="text" select="$subst4" />
            <xsl:with-param name="replace" select="'_'" />
            <xsl:with-param name="with" select="'\_'" />
            <xsl:with-param name="disable-output-escaping" select="no" />
          </xsl:call-template>
        </xsl:variable>
        <xsl:variable name="subst6">
          <xsl:call-template name="str:subst">
            <xsl:with-param name="text" select="$subst5" />
            <xsl:with-param name="replace" select="'$'" />
            <xsl:with-param name="with" select="'\$'" />
            <xsl:with-param name="disable-output-escaping" select="no" />
          </xsl:call-template>
        </xsl:variable>
        <xsl:variable name="subst7">
          <xsl:call-template name="str:subst">
            <xsl:with-param name="text" select="$subst6" />
            <xsl:with-param name="replace" select="'%'" />
            <xsl:with-param name="with" select="'\%'" />
            <xsl:with-param name="disable-output-escaping" select="no" />
          </xsl:call-template>
        </xsl:variable>
        <xsl:variable name="subst8">
          <xsl:call-template name="str:subst">
            <xsl:with-param name="text" select="$subst7" />
            <xsl:with-param name="replace" select="'#'" />
            <xsl:with-param name="with" select="'\#'" />
            <xsl:with-param name="disable-output-escaping" select="no" />
          </xsl:call-template>
        </xsl:variable>
        <xsl:variable name="subst9">
          <xsl:call-template name="str:subst">
            <xsl:with-param name="text" select="$subst8" />
            <xsl:with-param name="replace" select="'µ'" />
            <xsl:with-param name="with" select="'$\mu$'" />
            <xsl:with-param name="disable-output-escaping" select="no" />
          </xsl:call-template>
        </xsl:variable>
        <xsl:variable name="subst10">
          <xsl:call-template name="str:subst">
            <xsl:with-param name="text" select="$subst9" />
            <xsl:with-param name="replace" select="'®'" />
            <xsl:with-param name="with" select="'\texorpdfstring{\textregistered}{}'" />
            <xsl:with-param name="disable-output-escaping" select="no" />
          </xsl:call-template>
        </xsl:variable>
        <xsl:variable name="quote">"</xsl:variable>
        <!-- preparation for pretty quotes: replace all double quotes _outside_ screen
             sections with "\QUOTE{}" strings, which the makefile will then replace
             with pretty quotes by invoking sed a few times. Unfortunately there are
             no regular expressions in XSLT so there's no other way. -->
        <xsl:variable name="subst11">
          <xsl:choose>
            <xsl:when test="(name(..)='screen') or (name(../..)='screen')
                         or (name(..)='programlisting') or (name(../..)='programlisting')
                         or (name(..)='literal') or (name(../..)='literal')
                           ">
              <xsl:value-of select="$subst10" />
            </xsl:when>
            <xsl:otherwise>
              <xsl:call-template name="str:subst">
                <xsl:with-param name="text" select="$subst10" />
                <xsl:with-param name="replace" select="$quote" />
                <xsl:with-param name="with" select="'\QUOTE{}'" />
                <xsl:with-param name="disable-output-escaping" select="no" />
              </xsl:call-template>
            </xsl:otherwise>
          </xsl:choose>
        </xsl:variable>
        <xsl:variable name="subst12">
          <xsl:call-template name="str:subst">
            <xsl:with-param name="text" select="$subst11" />
            <xsl:with-param name="replace" select="'~'" />
            <xsl:with-param name="with" select="'\textasciitilde{}'" />
            <xsl:with-param name="disable-output-escaping" select="no" />
          </xsl:call-template>
        </xsl:variable>
        <xsl:variable name="subst13">
          <xsl:call-template name="str:subst">
            <xsl:with-param name="text" select="$subst12" />
            <xsl:with-param name="replace" select="'&amp;'" />
            <xsl:with-param name="with" select="'\&amp;'" />
            <xsl:with-param name="disable-output-escaping" select="no" />
          </xsl:call-template>
        </xsl:variable>
        <xsl:variable name="subst14">
          <xsl:call-template name="str:subst">
            <xsl:with-param name="text" select="$subst13" />
            <xsl:with-param name="replace" select="'→'" />
            <xsl:with-param name="with" select="'\ensuremath{\rightarrow}'" />
            <xsl:with-param name="disable-output-escaping" select="no" />
          </xsl:call-template>
        </xsl:variable>
        <xsl:variable name="subst15">
          <xsl:call-template name="str:subst">
            <xsl:with-param name="text" select="$subst14" />
            <xsl:with-param name="replace" select="'←'" />
            <xsl:with-param name="with" select="'\ensuremath{\leftarrow}'" />
            <xsl:with-param name="disable-output-escaping" select="no" />
          </xsl:call-template>
        </xsl:variable>
        <xsl:variable name="subst16">
          <xsl:call-template name="str:subst">
            <xsl:with-param name="text" select="$subst15" />
            <xsl:with-param name="replace" select="'↔'" />
            <xsl:with-param name="with" select="'\ensuremath{\leftrightarrow}'" />
            <xsl:with-param name="disable-output-escaping" select="no" />
          </xsl:call-template>
        </xsl:variable>
        <xsl:value-of select="$subst16" />
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!--
       xsltprocNewlineOutputHack - emits a single new line.

       Hack Alert! This template helps xsltproc split up the output text elements
                   and avoid reallocating them into the MB range. Calls to this
                   template is made occationally while generating larger output
                   file.  It's not necessary for small stuff like header.

                   The trick we're playing on xsltproc has to do with CDATA
                   and/or the escape setting of the xsl:text element.  It forces
                   xsltproc to allocate a new output element, thus preventing
                   things from growing out of proportions and slowing us down.

                   This was successfully employed to reduce a 18+ seconds run to
                   around one second (possibly less due to kmk overhead).
   -->
  <xsl:template name="xsltprocNewlineOutputHack">
      <xsl:text disable-output-escaping="yes"><![CDATA[
]]></xsl:text>
  </xsl:template>

</xsl:stylesheet>

