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

#include "label.h"
#include "labelwindow.h"
#include "control.h"
#include "pngreader.h"

#define MAX_ALPHA 0x3F

Label::Label(screen_context_t context, int x, int y, unsigned width, unsigned height, char *imageFile)
	: m_x(x)
	, m_y(y)
	, m_width(width)
	, m_height(height)
	, m_window(0)
	, m_alpha(0)
{
	FILE *file = 0;

	if (imageFile) {
		file = fopen(imageFile, "rb");
	}

	if (file) {
		// We have an image
		m_png = new PNGReader(file, context, MAX_ALPHA);
		if (m_png && m_png->doRead()) {
			m_window = LabelWindow::create(context, width, height);
			m_window->draw(*m_png);
		}
	}
}

Label::~Label()
{
	m_control = 0;
	if (m_window) delete m_window;
	if (m_png)    delete m_png;
	m_window = 0;
	m_png    = 0;
}

void Label::draw(screen_window_t window, int x, int y)
{
	if (!m_window)
		return;
	m_window->showAt(window, m_x+x, m_y+y);
}

void Label::hide(bool bHide)
{
	if (!m_window)
		return;

	m_window->setVisible(!bHide);
}

void Label::move(int x, int y)
{
	if (!m_window)
		return;
	m_window->move(m_x+x, m_y+y);
}

void Label::adjustAlpha(unsigned char alpha)
{
	if (!m_window || !m_png || (alpha == m_alpha))
		return;

	m_alpha = m_png->adjustAlpha(alpha);
	m_window->draw(*m_png);
}
