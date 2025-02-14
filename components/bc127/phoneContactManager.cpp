#include "phoneContactManager.h"
#include <algorithm>

PhoneContact::PhoneContact(const std::string &name, const std::string &number)
    : name_(name), number_(number) {}

std::string PhoneContact::get_name() const {
  return name_;
}

std::string PhoneContact::get_number() const {
  return number_;
}

std::string PhoneContact::to_string() const {
  return name_ + ":" + number_;
}

void PhoneContactManager::add_contact(const PhoneContact &contact) {
  contacts_.push_back(contact);
}

void PhoneContactManager::remove_contact(const PhoneContact &contact) {
    auto it = std::remove(this->contacts_.begin(), this->contacts_.end(), contact);
    if (it != this->contacts_.end()) {
        this->contacts_.erase(it, this->contacts_.end());
    }
}

void PhoneContactManager::clear_contacts() {
  contacts_.clear();
}

const std::vector<PhoneContact>& PhoneContactManager::get_contacts() const {
  return contacts_;
}

std::string normalizePhone(const std::string &phone) {
    std::string normalized;
    for (char c : phone) {
        if (std::isdigit(static_cast<unsigned char>(c))) {
            normalized.push_back(c);
        }
    }
    return normalized;
}

PhoneContact* PhoneContactManager::find_contact_by_number(const std::string &number) {
    std::string searchDigits = normalizePhone(number);
    
    for (auto &contact : contacts_) {
        std::string contactDigits = normalizePhone(contact.get_number());
        if (contactDigits.find(searchDigits) != std::string::npos ||
            searchDigits.find(contactDigits) != std::string::npos) {
            return &contact;
        }
    }
    return nullptr;
}

PhoneContact* PhoneContactManager::find_contact_by_name(const std::string &name) {
  for (auto &contact : contacts_) {
    if (contact.get_name() == name) {
      return &contact;  // Retorna un puntero al contacto encontrado
    }
  }
  return nullptr;  // Retorna nullptr si no encuentra el número
}
