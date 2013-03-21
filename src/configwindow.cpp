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

#include "configwindow.h"
#include "touchcontroloverlay_priv.h"
#include "control.h"
#include "logging.h"
#include <bps/bps.h>
#include <bps/screen.h>
#include <bps/event.h>
#include <bps/navigator.h>
#include <gestures/types.h>
#include <gestures/long_press.h>
#include <gestures/tap.h>
#include <gestures/double_tap.h>
#include <gestures/set.h>
#include <input/screen_helpers.h>
#include <unistd.h>

static unsigned char g_alpha = 0x7F;

/**
 * The callback invoked when a gesture is recognized or updated.
 */
void gesture_callback(gesture_base_t* gesture, mtouch_event_t* event, void* param, int async)
{
    switch (gesture->type) {
        case GESTURE_DOUBLE_TAP: {
            gesture_tap_t* d_tap = (gesture_tap_t*)gesture;
            SLOG("Double tap x:%d y:%d", d_tap->touch_coords.x, d_tap->touch_coords.y);

            g_alpha += 0x40;
            break;
        }
        case GESTURE_LONG_PRESS: {
        	gesture_long_press_t *lpress = (gesture_long_press_t *)gesture;
        	SLOG("Long Press x:%d y%d", lpress->coords.x, lpress->coords.y);

        	break;
        }
        default: {
            SLOG("Unknown Gesture");
            break;
        }
    }
    fprintf(stderr,"\n");
}

/**
 * Initialize the gestures sets
 */
void ConfigWindow::init_gestures(void)
{
    m_gset = gestures_set_alloc();
    if (NULL != m_gset) {
        double_tap_gesture_alloc(NULL, gesture_callback, m_gset);
        long_press_gesture_alloc(NULL, gesture_callback, m_gset);
    } else {
        SLOG("Failed to allocate gestures set\n");
    }
}

void ConfigWindow::cleanup_gestures(void)
{
    if (NULL != m_gset) {
        gestures_set_free(m_gset);
        m_gset = NULL;
    }
}

ConfigWindow* ConfigWindow::createConfigWindow(screen_context_t context, screen_window_t parent)
{
	const int zOrder = 10; // FIXME: hardcoded
	ConfigWindow *window = new ConfigWindow(context, parent);
	if (!window->m_valid) {
		delete window;
		return 0;
	}

	if (!window->setZOrder(zOrder) ||
			!window->setTouchSensitivity(true)) {
		delete window;
		return 0;
	}

	return window;
}

screen_buffer_t ConfigWindow::draw(TCOContext *emuContext)
{
	screen_buffer_t buffer;
	unsigned char *pixels;
	int stride;
	int i=0,j=0;

	if (!getPixels(&buffer, &pixels, &stride)) {
		return 0;
	}

	// Fill ConfigWindow background color
	for (i=0; i<m_size[1]; i++) {
		for (j=0; j<m_size[0]; j++) {
			pixels[i*stride+j*4]   = 0x30;
			pixels[i*stride+j*4+1] = 0x30;
			pixels[i*stride+j*4+2] = 0x30;
			pixels[i*stride+j*4+3] = 0xa0;
		}
	}

	if (m_hidden == false)
	{
		emuContext->drawControls(buffer);
	}
	return buffer;
}


void ConfigWindow::runEventLoop(TCOContext *emuContext)
{
	screen_buffer_t buffer;

	bool showingWindow = true;
	bps_initialize();
	screen_request_events(m_context);
	bps_event_t *event = NULL; // FIXME: How do we verify they ran bps_initialize?
	screen_event_t se;
	mtouch_event_t mtouch_event;
	int eventType;
	int contactId;
	bool touching = false;
	bool releasedThisRound = false;
	int startPos[2] = {0,0};
	int endPos[2] = {0,0};
	bool scaling = false;
	int  rc;
	unsigned char *pixels;
	int stride;

	init_gestures();

	while (showingWindow)
	{
		const int dirtyRects[4] = {0, 0, m_size[0], m_size[1]};

		releasedThisRound = false;
		bps_get_event(&event, 0);
		while (showingWindow && event)
		{
			int domain = bps_event_get_domain(event);
			if (domain == navigator_get_domain())
			{
				if (bps_event_get_code(event) == NAVIGATOR_SWIPE_DOWN)
					showingWindow = false;
				else if (bps_event_get_code(event) == NAVIGATOR_EXIT) {
					showingWindow = false;
				}
			} else if (domain == screen_get_domain()) {
				se = screen_event_get_event(event);
				screen_get_event_property_iv(se, SCREEN_PROPERTY_TYPE, &eventType);
				screen_get_event_property_iv(se, SCREEN_PROPERTY_TOUCH_ID, &contactId);

			    if( eventType == SCREEN_EVENT_MTOUCH_TOUCH ||
			    	eventType == SCREEN_EVENT_MTOUCH_MOVE ||
			    	eventType == SCREEN_EVENT_MTOUCH_RELEASE)
			    {
			        rc = screen_get_mtouch_event(se, &mtouch_event, 1);
			        if (rc)
			        {
			            SLOG("Error: failed to get mtouch event\n");
			        }

			        rc = gestures_set_process_event(m_gset, &mtouch_event, NULL);

			        if (!rc)
			        {
						switch (eventType)
						{
						case SCREEN_EVENT_MTOUCH_TOUCH:
							screen_get_event_property_iv(se, SCREEN_PROPERTY_TOUCH_ID, &contactId);
							if (contactId == 0 && !touching && !releasedThisRound) {
								touching = true;
								screen_get_event_property_iv(se, SCREEN_PROPERTY_SOURCE_POSITION, startPos);
								endPos[0]   = startPos[0];
								endPos[1]   = startPos[1];
							}
							break;
						case SCREEN_EVENT_MTOUCH_MOVE:
							screen_get_event_property_iv(se, SCREEN_PROPERTY_TOUCH_ID, &contactId);
							if (contactId == 0 && touching) {
								screen_get_event_property_iv(se, SCREEN_PROPERTY_SOURCE_POSITION, endPos);
							}
							break;
						case SCREEN_EVENT_MTOUCH_RELEASE:
							screen_get_event_property_iv(se, SCREEN_PROPERTY_TOUCH_ID, &contactId);
							if (contactId == 0 && touching) {
								touching = false;
								releasedThisRound = true;
								screen_get_event_property_iv(se, SCREEN_PROPERTY_SOURCE_POSITION, endPos);
							}
							break;
						default:
							break;
						}
			        }
			        else
			        {
			        	emuContext->adjustLabelsAlpha(g_alpha);
			        }
			    }
			}

			bps_get_event(&event, 0);
		}

		if (releasedThisRound) {
			m_selected = 0;
		} else if (touching) {
			if (!m_selected)
				m_selected = emuContext->controlAt(startPos);
			if (m_selected && (endPos[0] != startPos[0] || endPos[1] != startPos[1])) {
				m_selected->move(endPos[0] - startPos[0], endPos[1] - startPos[1], (unsigned*)m_size);
				startPos[0] = endPos[0];
				startPos[1] = endPos[1];
			}
		}

		buffer = draw(emuContext);
		if (buffer)
			post(buffer);
	}

	cleanup_gestures();
}
