#pragma once

namespace firmware {

// @model User
struct User {
  // @model User.userId
  const char* userId;
  // @model User.name
  const char* name;
  // @model User.paymentMethod
  const char* paymentMethod;
};

}  // namespace firmware
