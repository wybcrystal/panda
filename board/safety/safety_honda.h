// board enforces
//   in-state
//      accel set/resume
//   out-state
//      cancel button

// these are set in the Honda safety hooks...this is the wrong place
int gas_interceptor_detected = 0;

// all commands: brake and steering
// if controls_allowed
//     allow all commands up to limit
// else
//     block all commands that produce actuation

static void honda_rx_hook(CAN_FIFOMailBox_TypeDef *to_push) {
  // state machine to enter and exit controls
  // 0x1A6 for the ILX, 0x296 for the Civic Touring
  if ((to_push->RIR>>21) == 0x1A6 || (to_push->RIR>>21) == 0x296) {
    int buttons = (to_push->RDLR & 0xE0) >> 5;
    if (buttons == 4 || buttons == 3) {
      controls_allowed = 1;
    } else if (buttons == 2) {
      controls_allowed = 0;
    }
  }

  // exit controls on brake press
  if ((to_push->RIR>>21) == 0x17C) {
    // bit 50
    if (to_push->RDHR & 0x200000) {
      controls_allowed = 0;
    }
  }

  // exit controls on gas press if interceptor
  if ((to_push->RIR>>21) == 0x201) {
    gas_interceptor_detected = 1;
    int gas = ((to_push->RDLR & 0xFF) << 8) | ((to_push->RDLR & 0xFF00) >> 8);
    if (gas > 328) {
      controls_allowed = 0;
    }
  }

  // exit controls on gas press if no interceptor
  if (!gas_interceptor_detected) {
    if ((to_push->RIR>>21) == 0x17C) {
      if (to_push->RDLR & 0xFF) {
        controls_allowed = 0;
      }
    }
  }
}

static int honda_tx_hook(CAN_FIFOMailBox_TypeDef *to_send) {
  // BRAKE: safety check
  if ((to_send->RIR>>21) == 0x1FA) {
    if (controls_allowed) {
      if ((to_send->RDLR & 0xFFFFFF3F) != to_send->RDLR) return 0;
    } else {
      if ((to_send->RDLR & 0xFFFF0000) != to_send->RDLR) return 0;
    }
  }

  // STEER: safety check
  if ((to_send->RIR>>21) == 0xE4 || (to_send->RIR>>21) == 0x194) {
    if (controls_allowed) {
      // all messages are fine here
    } else {
      if ((to_send->RDLR & 0xFFFF0000) != to_send->RDLR) return 0;
    }
  }

  // GAS: safety check
  if ((to_send->RIR>>21) == 0x200) {
    if (controls_allowed) {
      // all messages are fine here
    } else {
      if ((to_send->RDLR & 0xFFFF0000) != to_send->RDLR) return 0;
    }
  }

  // 1 allows the message through
  return true;
}

static int honda_tx_lin_hook(int lin_num, uint8_t *data, int len) {
  // TODO: add safety if using LIN
  return true;
}

static void honda_init() {
}

const safety_hooks honda_hooks = {
  .init = honda_init,
  .rx = honda_rx_hook,
  .tx = honda_tx_hook,
  .tx_lin = honda_tx_lin_hook,
};

