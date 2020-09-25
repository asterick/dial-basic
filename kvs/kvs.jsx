/*
ISC License

Copyright (c) 2020, Bryon Vandiver

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

import React from "react";
import classnames from "classnames";

import TranslateEventCode from "./keycodes.js"
import style from "./style.css";

export default class KVSDisplay extends React.Component {
	constructor(props) {
		super(props);

		this.state = {
			blur: false,
			focus: true,
			active: true,

			locked: false,
			mouse: { x: 0, y: 0, scroll: 0, buttons: 0 },
			keys: []
		};

		this.videoRef = React.createRef();
		this.topElement = React.createRef();

		this.pointerLock = this.pointerLock.bind(this);
		this.lockChangeAlert = this.lockChangeAlert.bind(this);

		this.mouseMovement = this.mouseMovement.bind(this);
		this.mouseWheel = this.mouseWheel.bind(this);
		this.mouseButtons = this.mouseButtons.bind(this);

		this.keyboardDown = this.keyboardDown.bind(this);
		this.keyboardUp = this.keyboardUp.bind(this);

		this.visChange = this.visChange.bind(this);
		this.focus = this.focus.bind(this);
		this.blur = this.blur.bind(this);
	}

	componentDidMount() {
		var constraints = { audio: true, video: { width: 1920, height: 1080  } };

		navigator.mediaDevices
			.getUserMedia(constraints)
			.then(function(mediaStream) {
				var video = document.querySelector("video");

				video.srcObject = mediaStream;
				video.onloadedmetadata = function(e) {
					video.play();
				};
			})
			.catch(function(err) {
				console.log(err.name + ": " + err.message);
			});

		document.addEventListener('pointerlockchange', this.lockChangeAlert, false);
		document.addEventListener('keydown', this.keyboardDown);
		document.addEventListener('keyup', this.keyboardUp);
		document.addEventListener('mouseup', this.mouseButtons);
		document.addEventListener('mousemove', this.mouseMovement);
		document.addEventListener('mousedown', this.mouseButtons);
		document.addEventListener('mouseup', this.mouseButtons);
		document.addEventListener('wheel', this.mouseWheel);

		document.addEventListener('visibilitychange', this.visChange);
		window.addEventListener('blur', this.blur);
		window.addEventListener('focus', this.focus);
	}

	componentWillUnmount() {
		document.removeEventListener('pointerlockchange', this.lockChangeAlert, false);
		document.removeEventListener('mousemove', this.mouseMovement);
		document.removeEventListener('keyup', this.keyboardUp);
		document.removeEventListener('mouseup', this.mouseButtons);
		document.removeEventListener('mousedown', this.mouseButtons);
		document.removeEventListener('mouseup', this.mouseButtons);
		document.removeEventListener('wheel', this.mouseWheel);

		document.removeEventListener('visibilitychange', this.visChange);
		window.removeEventListener('blur', this.blur);
		window.removeEventListener('focus', this.focus);
	}

	/* Here we have our mouse state tracking */
	pointerLock(e) {
		if (!e.target.requestPointerLock) {
			return ;
		}

		e.target.requestPointerLock();
	}

	lockChangeAlert(e) {
		const locked = document.pointerLockElement != null;
		this.setState({ locked });
	}

	toggleFullscreen() {
		const elem = this.topElement.current;

		if (!document.fullscreenElement) {
			elem.requestFullscreen().catch(err => {
			  alert(`Error attempting to enable full-screen mode: ${err.message} (${err.name})`);
			});
		} else {
			document.exitFullscreen();
		}
	}

	/* Blur event */
	visChange(e) {
		const blur = document.visibilityState !== 'visible';
		const active = !blur && this.state.focus;
		const keys = active ? {} : this.state.keys;

		this.setState({keys, blur, active})
	}

	focus(e) {
		this.setState({focus: true, active: !this.state.blur})
	}

	blur(e) {
		this.setState({keys: [], focus: false, active: false})
	}

	/* Update device */
	updateMouse(mouse) {
		const {x,y,scroll,buttons} = mouse;
		mouse.x = 0;
		mouse.y = 0;
		mouse.scroll = 0;
		this.props.bluetooth.writeMouse(x,y,scroll,buttons);
	}

	updateKeyboard() {
		let keys = Object.keys(this.state.keys).map((k) => k|0);
		keys.sort();

		this.props.bluetooth.writeKeys(keys);
	}

	/* Mouse handlers */
	mouseMovement(e) {
		if (!this.state.locked) return false;

		const mouse = this.state.mouse;
		mouse.x += e.movementX;
		mouse.y += e.movementY;
		this.updateMouse(mouse);
		this.setState(mouse);
	}

	mouseWheel(e) {
		if (!this.state.locked) return false;

		const mouse = this.state.mouse;
		mouse.scroll += e.deltaY;
		this.updateMouse(mouse);
		this.setState(mouse);
	}

	mouseButtons(e) {
		if (!this.state.locked) return false;

		const mouse = this.state.mouse;
		mouse.buttons = e.buttons;
		this.updateMouse(mouse);
		this.setState(mouse);
	}

	/* Keyboard handlers */
	keyboardDown(e) {
		const code = TranslateEventCode[e.code];
		if (!code) return false ;

		e.preventDefault();

		const keys = this.state.keys;
		keys[code] = true;
		this.setState(keys);

		this.updateKeyboard();
	}

	keyboardUp(e) {
		const code = TranslateEventCode[e.code];
		if (!code) return false ;

		e.preventDefault();

		const keys = this.state.keys;
		delete keys[code];
		this.setState(keys);

		this.updateKeyboard();
	}

	/* View template */
	render() {
        return <video
            autoPlay={true}
            ref={this.videoRef}
            onClick={this.pointerLock}
            className={classnames({
                [style.video]: true,
                [style.blur]: !this.state.active
            })}
            />
    }
}
