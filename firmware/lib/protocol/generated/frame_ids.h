#pragma once
#include <stdint.h>

namespace firmware {

enum class FrameId : uint8_t {
  // @model frame:rent.request
  RENT_REQUEST = 16,
  // @model frame:rent.reserve
  RENT_RESERVE = 17,
  // @model frame:rent.reserved
  RENT_RESERVED = 18,
  // @model frame:rent.unlock
  RENT_UNLOCK = 19,
  // @model frame:rent.started
  RENT_STARTED = 20,
  // @model frame:rent.active
  RENT_ACTIVE = 21,
  // @model frame:rent.receipt
  RENT_RECEIPT = 22,
  // @model frame:rent.showReceipt
  RENT_SHOWRECEIPT = 23,
  // @model frame:return.request
  RETURN_REQUEST = 32,
  // @model frame:return.docked
  RETURN_DOCKED = 33,
  // @model frame:return.paymentRequest
  RETURN_PAYMENTREQUEST = 34,
  // @model frame:return.confirmPayment
  RETURN_CONFIRMPAYMENT = 35,
  // @model frame:return.lock
  RETURN_LOCK = 36,
  // @model frame:return.available
  RETURN_AVAILABLE = 37,
  // @model frame:return.receipt
  RETURN_RECEIPT = 38,
  // @model frame:return.close
  RETURN_CLOSE = 39,
  // @model frame:return.ended
  RETURN_ENDED = 40,
};

}  // namespace firmware
