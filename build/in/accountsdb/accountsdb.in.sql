CREATE DATABASE IF NOT EXISTS `accounts` /*!40100 DEFAULT CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci */;

use accounts;

CREATE TABLE IF NOT EXISTS `account` (
  `row_id` bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  `user_id` varchar(64) COLLATE utf8mb4_unicode_ci NOT NULL,
  `email` varchar(320) COLLATE utf8mb4_unicode_ci NOT NULL,
  `admin_key` varchar(256) COLLATE utf8mb4_unicode_ci NOT NULL,
  `created` datetime NOT NULL,
  `deleted` datetime NULL,
  PRIMARY KEY (`row_id`),
  UNIQUE KEY `account_user_id_unq` (`user_id`),
  UNIQUE KEY `account_admin_key_unq` (`admin_key`)
) ENGINE=InnoDB AUTO_INCREMENT=1000000 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS `worker` (
  `id` bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  `account_row_id` bigint(20) unsigned NOT NULL,
  `name` varchar(50) COLLATE utf8mb4_unicode_ci NOT NULL,
  `version` bigint(20) unsigned NOT NULL,
  `user_key` varchar(256) COLLATE utf8mb4_unicode_ci NOT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `account_worker_name_unq` (`account_row_id`, `name`),
  KEY `fk_account_row_id_idx` (`account_row_id`),
  UNIQUE KEY `worker_user_key_unq` (`user_key`),
  CONSTRAINT `fk_account_row_id` FOREIGN KEY (`account_row_id`) REFERENCES `account` (`row_id`) ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB AUTO_INCREMENT=5000000 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS `worker_index` (
	`worker_id` bigint(20) unsigned NOT NULL,
    `index` blob NOT NULL,
    PRIMARY KEY (`worker_id`),
    KEY `fk_worker_id_idx` (`worker_id`),
    CONSTRAINT `fk_worker_id` FOREIGN KEY (`worker_id`) REFERENCES `worker` (`id`) ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS `worker_type_definition` (
    `worker_id` bigint(20) unsigned NOT NULL,
    `compressed_type_definitions` blob NOT NULL,
    PRIMARY KEY (`worker_id`),
    KEY `fk_wtd_worker_id_idx` (`worker_id`),
    CONSTRAINT `fk_wtd_worker_id` FOREIGN KEY (`worker_id`) REFERENCES `worker` (`id`) ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;


