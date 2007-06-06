/*
 *  Copyright (c) 2007 Cyrille Berger <cberger@cberger.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef _KIS_EXIV2_IO_H_
#define _KIS_EXIV2_IO_H_

#include <kis_meta_data_io_backend.h>

class KisExiv2IO : public KisMetaData::IOBackend {
    struct Private;
    public:
        KisExiv2IO();
        virtual ~KisExiv2IO() {}
        virtual BackendType type() { return Binary; }
        virtual bool supportSaving() { return true; }
        virtual bool saveTo(KisMetaData::Store* store, QIODevice* ioDevice);
        virtual bool canSaveAllEntries(KisMetaData::Store* store);
        virtual bool supportLoading() { return true; }
        virtual bool loadFrom(KisMetaData::Store* store, QIODevice* ioDevice);
    private:
        Private* const d;
};

#endif
