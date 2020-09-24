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

import React, { Component } from "react";
import ReactDOM from "react-dom";

import style from "./style.less";

import TranslateEventCode from "./keycodes.js"

export default class Root extends React.Component {
	constructor(props) {
		super(props);
		
		this.state = {
			locked: false,
			mouse: { x: 0, y: 0, scroll: 0, buttons: 0 },
			keys: {}
		};

		this.videoRef = React.createRef();
		
		this.pointerLock = this.pointerLock.bind(this);
		this.lockChangeAlert = this.lockChangeAlert.bind(this);

		this.mouseMovement = this.mouseMovement.bind(this);
		this.mouseWheel = this.mouseWheel.bind(this);
		this.mouseButtons = this.mouseButtons.bind(this);
		
		this.keyboardDown = this.keyboardDown.bind(this);
		this.keyboardUp = this.keyboardUp.bind(this);
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
	}

	componentWillUnmount() {
		document.removeEventListener('pointerlockchange', this.lockChangeAlert, false);
		document.removeEventListener('mousemove', this.mouseMovement);
		document.removeEventListener('keyup', this.keyboardUp);
		document.removeEventListener('mouseup', this.mouseButtons);
		document.removeEventListener('mousedown', this.mouseButtons);
		document.removeEventListener('mouseup', this.mouseButtons);
		document.removeEventListener('wheel', this.mouseWheel);
	}

	/* Here we have our mouse state tracking */
	pointerLock(e) {
		if (!e.target.requestPointerLock) {
			return ;
		}
		
		e.target.requestPointerLock()
	}

	lockChangeAlert(e) {
		const locked = document.pointerLockElement != null;
		this.setState({ locked });
	}

	/* Update device */
	updateMouse() {
		//console.log(this.state.mouse)
	}

	updateKeyboard() {
		for (let k of Object.keys(this.state.keys)) {
			if (!TranslateEventCode[k]) console.log(k)
		}

		let keys = Object.keys(this.state.keys).map((k) => TranslateEventCode[k]);
		let modifier = 0;

		for (let i = 0; i < 8; i++) {
			const idx = keys.indexOf(i+224);
			if (idx < 0) continue ;

			keys.splice(idx, 1);
			modifier |= 1 << i;
		}

		keys.sort();

		console.log(modifier.toString(2), keys)
	}

	/* Mouse handlers */
	mouseMovement(e) {
		if (!this.state.locked) return false;

		const mouse = this.state.mouse;
		mouse.x += e.movementX;
		mouse.y += e.movementY;
		this.setState(mouse);
		this.updateMouse();
	}

	mouseWheel(e) {
		if (!this.state.locked) return false;

		const mouse = this.state.mouse;
		mouse.scroll += e.deltaY;
		this.setState(mouse);
		this.updateMouse();
	}

	mouseButtons(e) {
		if (!this.state.locked) return false;

		const mouse = this.state.mouse;
		mouse.buttons = e.buttons;
		this.setState(mouse);
		this.updateMouse();
	}

	/* Keyboard handlers */
	keyboardDown(e) {
		e.preventDefault();
		
		const keys = this.state.keys;
		keys[e.code] = true;
		this.setState(keys);

		this.updateKeyboard();
	}

	keyboardUp(e) {
		e.preventDefault();

		const keys = this.state.keys;
		delete keys[e.code];
		this.setState(keys);

		this.updateKeyboard();
	}

	/* View template */
	render() {
		return <div>
			<video onClick={this.pointerLock} className={style.video} autoPlay={true} ref={this.videoRef}></video>
		</div>;
	}
}

async function main() {
	ReactDOM.render(<Root />, document.querySelector(".container"));
}

main();
