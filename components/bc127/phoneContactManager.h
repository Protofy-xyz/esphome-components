#pragma once

#include <string>
#include <vector>

    class PhoneContact
    {
    public:
        PhoneContact(const std::string &name, const std::string &number);

        std::string get_name() const;
        std::string get_number() const;
        std::string to_string() const;


    private:
        std::string name_;
        std::string number_;
    };

    class PhoneContactManager
    {
    public:
        // Agrega un contacto a la lista
        void add_contact(const PhoneContact &contact);

        // Quita un contacto de la lista
        void remove_contact(const PhoneContact &contact);

        // Limpia la lista de contactos
        void clear_contacts();

        // Retorna la lista de contactos
        const std::vector<PhoneContact> &get_contacts() const;
        
        PhoneContact* find_contact_by_number(const std::string &number);

    private:
        std::vector<PhoneContact> contacts_;
    };