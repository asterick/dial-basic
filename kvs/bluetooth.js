const DIAL_BASIC_SERVICE = '5a42ba50-8570-3aa1-ea11-71f8082c3f68';
const options = {
    filters: [
        { services: [DIAL_BASIC_SERVICE] }
    ]
};

export class Bluetooth {
    constructor() {
        this.keyboard_queue = [];
        this.mouse_dirty = false;
        this.gatt_busy = false;
    }

    get connected() {
        return this.gatt && this.gatt.connected;
    }

    async connect() {
        this.device = await navigator.bluetooth.requestDevice(options);
        this.gatt = await this.device.gatt.connect();
        const service = await this.gatt.getPrimaryService(DIAL_BASIC_SERVICE);
        const characteristics = await service.getCharacteristics();

        this.led_char = characteristics[2];
        this.kbd_char = characteristics[3];
        this.mouse_char = characteristics[4];

        // Subscribe to LED updates
        const notice_led = await this.led_char.startNotifications();
        this.led_char.addEventListener('characteristicvaluechanged', this.led_update.bind(this));
    }

    writeKeys(keys) {
        this.keyboard_queue.push(new Uint8Array(keys.concat(0)));
        this.send_gatt();
    }

    writeMouse(x,y,scroll,buttons) {
        this.mouseX += x;
        this.mouseY += y;
        this.mouseScroll += scroll;
        this.mouse_dirty = (this.mouseButtons != buttons) || this.mouseX || this.mouseY || this.mouseScroll;
        this.mouseButtons = buttons;

        this.send_gatt();
    }

    async send_gatt() {
        if (this.gatt_busy) return ;
        this.gatt_busy = true;

        while (true) {
            try {
                if (!this.connected) {
                    this.keyboard_queue = [];
                    this.mouseX = 0;
                    this.mouseY = 0;
                    this.mouseScroll = 0;
                    break ;
                } else if (this.keyboard_queue.length > 0) {
                    await this.kbd_char.writeValueWithoutResponse(this.keyboard_queue.shift());
                } else if (this.mouse_dirty) {
                    const val = new Uint8Array([
                        this.mouseX & 0xFF,
                        this.mouseY & 0xFF,
                        this.mouseScroll & 0xFF,
                        this.mouseButtons
                    ]);

                    this.mouseX = 0;
                    this.mouseY = 0;
                    this.mouseScroll = 0;
                    this.mouse_dirty = false;

                    await this.mouse_char.writeValueWithoutResponse(val);
                } else {
                    break ;
                }
            } catch (e) {
                console.error(e);
                break ;
            }
        }

        this.gatt_busy = false;
    }

    led_update(e) {
        this.led_value = e.target.value.getUint8(0);
        console.log(this.led_value);
    }
}
