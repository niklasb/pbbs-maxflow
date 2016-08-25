class CreatePbbsPrograms < ActiveRecord::Migration
  def self.up
    create_table :pbbs_programs do |t|

      t.timestamps
    end
  end

  def self.down
    drop_table :pbbs_programs
  end
end
