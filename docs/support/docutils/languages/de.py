# Authors:   David Goodger; Gunnar Schwant
# Contact:   goodger@users.sourceforge.net
# Revision:  $Revision: 21817 $
# Date:      $Date: 2005-07-21 14:39:57 -0600 (Thu, 21 Jul 2005) $
# Copyright: This module has been placed in the public domain.

# New language mappings are welcome.  Before doing a new translation, please
# read <http://docutils.sf.net/docs/howto/i18n.html>.  Two files must be
# translated for each language: one in docutils/languages, the other in
# docutils/parsers/rst/languages.

"""
German language mappings for language-dependent features of Docutils.
"""

__docformat__ = 'reStructuredText'

labels = {
    'author': 'Autor',
    'authors': 'Autoren',
    'organization': 'Organisation',
    'address': 'Adresse',
    'contact': 'Kontakt',
    'version': 'Version',
    'revision': 'Revision',
    'status': 'Status',
    'date': 'Datum',
    'dedication': 'Widmung',
    'copyright': 'Copyright',
    'abstract': 'Zusammenfassung',
    'attention': 'Achtung!',
    'caution': 'Vorsicht!',
    'danger': '!GEFAHR!',
    'error': 'Fehler',
    'hint': 'Hinweis',
    'important': 'Wichtig',
    'note': 'Bemerkung',
    'tip': 'Tipp',
    'warning': 'Warnung',
    'contents': 'Inhalt'}
"""Mapping of node class name to label text."""

bibliographic_fields = {
    'autor': 'author',
    'autoren': 'authors',
    'organisation': 'organization',
    'adresse': 'address',
    'kontakt': 'contact',
    'version': 'version',
    'revision': 'revision',
    'status': 'status',
    'datum': 'date',
    'copyright': 'copyright',
    'widmung': 'dedication',
    'zusammenfassung': 'abstract'}
"""German (lowcased) to canonical name mapping for bibliographic fields."""

author_separators = [';', ',']
"""List of separator strings for the 'Authors' bibliographic field. Tried in
order."""
