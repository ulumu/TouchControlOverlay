/*
 * Copyright (c) 2011 Research In Motion Limited.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef PNGREADER_H_
#define PNGREADER_H_

#include <png.h>
#include <screen/screen.h>

class PNGReader
{
public:
	PNGReader(FILE *file, screen_context_t context);
	PNGReader(FILE *file, screen_context_t context, unsigned char minAlpha);
	~PNGReader();

	bool doRead();

	screen_pixmap_t  getScreenPixmap(void) { return m_pixmap; }
	screen_buffer_t  getScreenBuffer(void) { return m_buffer; }
	int              getWidth(void)        { return m_width;  }
	int              getHeight(void)       { return m_height; }
	int              getStride(void)       { return m_stride; }

private:
	screen_context_t m_context;
	screen_pixmap_t  m_pixmap;
	screen_buffer_t  m_buffer;

	png_structp      m_read;
	png_infop        m_info;
	unsigned char   *m_data;
	png_bytep       *m_rows;

	int              m_width;
	int              m_height;
	int              m_stride;
	FILE            *m_file;

	unsigned char    m_maxAlpha;
};

#endif /* PNGREADER_H_ */
