<?php

declare(strict_types=1);

namespace DoctrineMigrations;

use Doctrine\DBAL\Schema\Schema;
use Doctrine\Migrations\AbstractMigration;

/**
 * Auto-generated Migration: Please modify to your needs!
 */
final class Version20260414130016 extends AbstractMigration
{
    public function getDescription(): string
    {
        return '';
    }

    public function up(Schema $schema): void
    {
        // this up() migration is auto-generated, please modify it to your needs
        $this->addSql('CREATE TABLE parking_spot (id INT AUTO_INCREMENT NOT NULL, name VARCHAR(50) NOT NULL, is_occupied TINYINT NOT NULL, last_updated_at DATETIME DEFAULT NULL, is_active TINYINT NOT NULL, PRIMARY KEY (id)) DEFAULT CHARACTER SET utf8mb4 COLLATE `utf8mb4_unicode_ci`');
        $this->addSql('CREATE TABLE payment (id INT AUTO_INCREMENT NOT NULL, amount NUMERIC(10, 2) NOT NULL, currency VARCHAR(3) NOT NULL, status VARCHAR(20) NOT NULL, stripe_payment_intent_id VARCHAR(255) DEFAULT NULL, created_at DATETIME NOT NULL, user_id INT NOT NULL, reservation_id INT DEFAULT NULL, subscription_id INT DEFAULT NULL, INDEX IDX_6D28840DA76ED395 (user_id), INDEX IDX_6D28840DB83297E7 (reservation_id), INDEX IDX_6D28840D9A1887DC (subscription_id), PRIMARY KEY (id)) DEFAULT CHARACTER SET utf8mb4 COLLATE `utf8mb4_unicode_ci`');
        $this->addSql('CREATE TABLE reservation (id INT AUTO_INCREMENT NOT NULL, start_time DATETIME NOT NULL, end_time DATETIME NOT NULL, status VARCHAR(20) NOT NULL, total_price NUMERIC(10, 2) NOT NULL, created_at DATETIME NOT NULL, user_id INT NOT NULL, parking_spot_id INT NOT NULL, INDEX IDX_42C84955A76ED395 (user_id), INDEX IDX_42C84955A31B2BA6 (parking_spot_id), PRIMARY KEY (id)) DEFAULT CHARACTER SET utf8mb4 COLLATE `utf8mb4_unicode_ci`');
        $this->addSql('CREATE TABLE sensor_data (id INT AUTO_INCREMENT NOT NULL, is_occupied TINYINT NOT NULL, distance_cm DOUBLE PRECISION DEFAULT NULL, esp32_id VARCHAR(50) DEFAULT NULL, recorded_at DATETIME NOT NULL, parking_spot_id INT NOT NULL, INDEX IDX_801762CCA31B2BA6 (parking_spot_id), PRIMARY KEY (id)) DEFAULT CHARACTER SET utf8mb4 COLLATE `utf8mb4_unicode_ci`');
        $this->addSql('CREATE TABLE subscription (id INT AUTO_INCREMENT NOT NULL, plan VARCHAR(20) NOT NULL, price NUMERIC(10, 2) NOT NULL, start_date DATETIME NOT NULL, end_date DATETIME NOT NULL, status VARCHAR(20) NOT NULL, stripe_subscription_id VARCHAR(255) DEFAULT NULL, user_id INT NOT NULL, INDEX IDX_A3C664D3A76ED395 (user_id), PRIMARY KEY (id)) DEFAULT CHARACTER SET utf8mb4 COLLATE `utf8mb4_unicode_ci`');
        $this->addSql('CREATE TABLE `user` (id INT AUTO_INCREMENT NOT NULL, email VARCHAR(180) NOT NULL, roles JSON NOT NULL, password VARCHAR(255) NOT NULL, first_name VARCHAR(100) NOT NULL, last_name VARCHAR(100) NOT NULL, created_at DATETIME NOT NULL, rgpd_consent_at DATETIME DEFAULT NULL, UNIQUE INDEX UNIQ_8D93D649E7927C74 (email), PRIMARY KEY (id)) DEFAULT CHARACTER SET utf8mb4 COLLATE `utf8mb4_unicode_ci`');
        $this->addSql('CREATE TABLE messenger_messages (id BIGINT AUTO_INCREMENT NOT NULL, body LONGTEXT NOT NULL, headers LONGTEXT NOT NULL, queue_name VARCHAR(190) NOT NULL, created_at DATETIME NOT NULL, available_at DATETIME NOT NULL, delivered_at DATETIME DEFAULT NULL, INDEX IDX_75EA56E0FB7336F0 (queue_name), INDEX IDX_75EA56E0E3BD61CE (available_at), INDEX IDX_75EA56E016BA31DB (delivered_at), PRIMARY KEY (id)) DEFAULT CHARACTER SET utf8mb4 COLLATE `utf8mb4_unicode_ci`');
        $this->addSql('ALTER TABLE payment ADD CONSTRAINT FK_6D28840DA76ED395 FOREIGN KEY (user_id) REFERENCES `user` (id)');
        $this->addSql('ALTER TABLE payment ADD CONSTRAINT FK_6D28840DB83297E7 FOREIGN KEY (reservation_id) REFERENCES reservation (id)');
        $this->addSql('ALTER TABLE payment ADD CONSTRAINT FK_6D28840D9A1887DC FOREIGN KEY (subscription_id) REFERENCES subscription (id)');
        $this->addSql('ALTER TABLE reservation ADD CONSTRAINT FK_42C84955A76ED395 FOREIGN KEY (user_id) REFERENCES `user` (id)');
        $this->addSql('ALTER TABLE reservation ADD CONSTRAINT FK_42C84955A31B2BA6 FOREIGN KEY (parking_spot_id) REFERENCES parking_spot (id)');
        $this->addSql('ALTER TABLE sensor_data ADD CONSTRAINT FK_801762CCA31B2BA6 FOREIGN KEY (parking_spot_id) REFERENCES parking_spot (id)');
        $this->addSql('ALTER TABLE subscription ADD CONSTRAINT FK_A3C664D3A76ED395 FOREIGN KEY (user_id) REFERENCES `user` (id)');
    }

    public function down(Schema $schema): void
    {
        // this down() migration is auto-generated, please modify it to your needs
        $this->addSql('ALTER TABLE payment DROP FOREIGN KEY FK_6D28840DA76ED395');
        $this->addSql('ALTER TABLE payment DROP FOREIGN KEY FK_6D28840DB83297E7');
        $this->addSql('ALTER TABLE payment DROP FOREIGN KEY FK_6D28840D9A1887DC');
        $this->addSql('ALTER TABLE reservation DROP FOREIGN KEY FK_42C84955A76ED395');
        $this->addSql('ALTER TABLE reservation DROP FOREIGN KEY FK_42C84955A31B2BA6');
        $this->addSql('ALTER TABLE sensor_data DROP FOREIGN KEY FK_801762CCA31B2BA6');
        $this->addSql('ALTER TABLE subscription DROP FOREIGN KEY FK_A3C664D3A76ED395');
        $this->addSql('DROP TABLE parking_spot');
        $this->addSql('DROP TABLE payment');
        $this->addSql('DROP TABLE reservation');
        $this->addSql('DROP TABLE sensor_data');
        $this->addSql('DROP TABLE subscription');
        $this->addSql('DROP TABLE `user`');
        $this->addSql('DROP TABLE messenger_messages');
    }
}
