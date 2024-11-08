#include "phoneContactManager.h"

PhoneContact::PhoneContact(const std::string &name, const std::string &number)
    : name_(name), number_(number) {}

std::string PhoneContact::get_name() const {
  return name_;
}

std::string PhoneContact::get_number() const {
  return number_;
}

void PhoneContactManager::add_contact(const PhoneContact &contact) {
  contacts_.push_back(contact);
}

void PhoneContactManager::clear_contacts() {
  contacts_.clear();
}

const std::vector<PhoneContact>& PhoneContactManager::get_contacts() const {
  return contacts_;
}

PhoneContact* PhoneContactManager::find_contact_by_number(const std::string &number) {
  for (auto &contact : contacts_) {
    if (contact.get_number() == number) {
      return &contact;  // Retorna un puntero al contacto encontrado
    }
  }
  return nullptr;  // Retorna nullptr si no encuentra el número
}