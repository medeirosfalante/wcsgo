#include <iostream>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "log.hpp"
#include "remotecall.hpp"
#include "wall.hpp"

using namespace std;

bool shouldGlow = true;

int main() {
    if (getuid() != 0) {
        cout << "You should run this as root." << endl;
        return 0;
    }

    cout << "s0beit linux wall version 1.3" << endl;

    log::init();
    log::put("wall loaded...");

	Display* dpy = XOpenDisplay(0);
	Window root = DefaultRootWindow(dpy);
	XEvent ev;

	int keycode = XKeysymToKeycode(dpy, XK_X);
	unsigned int modifiers = ControlMask | ShiftMask;

	XGrabKey(dpy, keycode, modifiers, root, false,
				GrabModeAsync, GrabModeAsync);
	XSelectInput(dpy, root, KeyPressMask);
	
    remotecall::Handle csgo;

    while (true) {
        if (remotecall::FindProcessByName("csgo_linux64", &csgo)) {
            break;
        }

        usleep(500);
    }

    remotecall::MapModuleMemoryRegion client;

    client.start = 0;

    while (client.start == 0) {
        if (!csgo.IsRunning()) {
            cout << "You close the game before find" << endl;
            return 0;
        }

        csgo.ParseMaps();

        for (auto region : csgo.regions) {
            if (region.filename.compare("client_client.so") == 0 && region.executable) {
                client = region;
                break;
            }
        }

        usleep(500);
    }

    client.client_start = client.start;

    void* foundGlowPointerCall = client.find(csgo,
                                             "\xE8\x00\x00\x00\x00\x48\x8b\x10\x48\xc1\xe3\x06\x44",
                                             "x????xxxxxxxx");

    
    unsigned long call = csgo.GetCallAddress(foundGlowPointerCall);

    unsigned int addressOfGlowPointerOffset;

    if (!csgo.Read((void*) (call + 0x10), &addressOfGlowPointerOffset, sizeof(unsigned int))) {
        cout << "cannout find address" << endl;
        return 0;
    }
    unsigned long addressOfGlowPointer = (call + 0x10) + addressOfGlowPointerOffset + 0x4  ;

    while (csgo.IsRunning()) {
		while (XPending(dpy) > 0) {
			XNextEvent(dpy, &ev);
			switch (ev.type) {
				case KeyPress:
					cout << "Toggling glow..." << endl;
					XUngrabKey(dpy, keycode, modifiers, root);
					shouldGlow = !shouldGlow;
					break;
				default:
					break;
			}

			XGrabKey(dpy, keycode, modifiers, root, false,
						GrabModeAsync, GrabModeAsync);
			XSelectInput(dpy, root, KeyPressMask);
		}

		if (shouldGlow)
	        wall::Glow(&csgo, &client, addressOfGlowPointer);

        usleep(1000);
    }

    return 0;
}