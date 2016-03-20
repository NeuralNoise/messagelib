/*
  Copyright (c) 2016 Montel Laurent <montel@kde.org>

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/
#ifndef WEBENGINESCRIPT_H
#define WEBENGINESCRIPT_H

#include <QString>
#include "messageviewer_export.h"

namespace MessageViewer
{
namespace WebEngineScript
{
MESSAGEVIEWER_EXPORT QString findAllImages();
MESSAGEVIEWER_EXPORT QString findAllScripts();
MESSAGEVIEWER_EXPORT QString findAllAnchors();
MESSAGEVIEWER_EXPORT QString findAllAnchorsAndForms();
}
}
#endif // WEBENGINESCRIPT_H
