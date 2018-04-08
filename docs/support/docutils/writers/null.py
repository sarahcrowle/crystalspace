# Author: David Goodger
# Contact: goodger@python.org
# Revision: $Revision: 21817 $
# Date: $Date: 2005-07-21 14:39:57 -0600 (Thu, 21 Jul 2005) $
# Copyright: This module has been placed in the public domain.

"""
A do-nothing Writer.
"""

from docutils import writers


class Writer(writers.Writer):

    supported = ('null',)
    """Formats this writer supports."""

    config_section = 'null writer'
    config_section_dependencies = ('writers',)

    def translate(self):
        pass
